//
// Created by Lauren on 29/03/24.
//

#include <wrtc/interfaces/native_connection.hpp>

#include <memory>
#include <utility>
#include <rtc_base/time_utils.h>
#include <wrtc/exceptions.hpp>
#include <wrtc/interfaces/reflector_relay_port_factory.hpp>


namespace wrtc::interfaces {
    NativeConnection::NativeConnection(std::vector<models::RTCServer> rtc_servers, const bool enable_p2p, const bool is_outgoing,  const utils::json& custom_parameters):
    custom_parameters_(custom_parameters),
    is_outgoing_(is_outgoing),
    enable_p2p_(enable_p2p),
    rtc_servers_(std::move(rtc_servers)),
    event_log_(std::make_unique<webrtc::RtcEventLogNull>()) {}

    void NativeConnection::open() {
        init_connection();
        content_negotiation_context_ = std::make_unique<ContentNegotiationContext>(
            environment(),
            is_outgoing_,
            factory_->media_engine(),
            factory_->ssrc_generator(),
            payload_type_suggester_.get()
        );
        content_negotiation_context_->copy_codecs_from_channel_manager(false);
        const std::weak_ptr weak(shared_from_this());
        network_thread().PostTask([weak] {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->start();
        });
    }

    webrtc::RelayPortFactoryInterface* NativeConnection::get_relay_port_factory() {
        const bool standalone_reflector_mode = get_custom_parameter_bool("network_standalone_reflectors");
        uint32_t standalone_reflector_role_id = 0;
        if (standalone_reflector_mode) {
            if (is_outgoing_) {
                standalone_reflector_role_id = 1;
            } else {
                standalone_reflector_role_id = 2;
            }
        }
        relay_port_factory_ = std::make_unique<ReflectorRelayPortFactory>(
            rtc_servers_,
            standalone_reflector_mode,
            standalone_reflector_role_id,
            underlying_socket_factory_
         );
        return relay_port_factory_.get();
    }

    std::pair<webrtc::ServerAddresses, std::vector<webrtc::RelayServerConfig>> NativeConnection::get_stun_and_turn_servers() {
        webrtc::ServerAddresses stun_servers;
        std::vector<webrtc::RelayServerConfig> turn_servers;
        for (auto &[id, host, port, login, password, isTurn, isTcp] : rtc_servers_) {
            if (isTurn) {
                turn_servers.emplace_back(
                    webrtc::SocketAddress(host, port),
                    login,
                    password,
                    isTcp ? webrtc::PROTO_TCP : webrtc::PROTO_UDP
                );
            } else {
                auto stun_address = webrtc::SocketAddress(host, port);
                stun_servers.insert(stun_address);
            }
        }
        return {stun_servers, turn_servers};
    }

    void NativeConnection::set_port_allocator_flags(webrtc::BasicPortAllocator* port_allocator) {
        uint32_t flags = port_allocator->flags();
        if (get_custom_parameter_bool("network_use_default_route")) {
            flags |= webrtc::PORTALLOCATOR_DISABLE_ADAPTER_ENUMERATION;
        }

        if (get_custom_parameter_bool("network_enable_shared_socket")) {
            flags |= webrtc::PORTALLOCATOR_ENABLE_SHARED_SOCKET;
        }
        flags |= webrtc::PORTALLOCATOR_ENABLE_IPV6;
        flags |= webrtc::PORTALLOCATOR_ENABLE_IPV6_ON_WIFI;
        flags |= webrtc::PORTALLOCATOR_DISABLE_TCP;
        if (!enable_p2p_) {
            flags |= webrtc::PORTALLOCATOR_DISABLE_UDP;
            flags |= webrtc::PORTALLOCATOR_DISABLE_STUN;
            uint32_t candidate_filter = port_allocator->candidate_filter();
            candidate_filter &= ~webrtc::CF_REFLEXIVE;
            port_allocator->SetCandidateFilter(candidate_filter);
        }
        port_allocator->set_step_delay(webrtc::kMinimumStepDelay);
        port_allocator->set_flags(flags);
    }

    webrtc::TimeDelta NativeConnection::get_regather_on_failed_networks_interval() {
        return webrtc::TimeDelta::Seconds(8);
    }

    webrtc::IceRole NativeConnection::ice_role() const {
        return is_outgoing_ ? webrtc::ICEROLE_CONTROLLING : webrtc::ICEROLE_CONTROLLED;
    }

    webrtc::IceMode NativeConnection::ice_mode() const {
        return webrtc::ICEMODE_FULL;
    }

    void NativeConnection::register_transport_callbacks(webrtc::P2PTransportChannel* transport_channel) {
        const std::weak_ptr weak(shared_from_this());
        transport_channel->SubscribeCandidateGathered(this, [weak](webrtc::IceTransportInternal*, const webrtc::Candidate &candidate) {
            assert(networkThread().IsCurrent());
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->signaling_thread().PostTask([weak, candidate] {
                const auto strong_signaling = std::static_pointer_cast<NativeConnection>(weak.lock());
                if (!strong_signaling) {
                    return;
                }
                webrtc::Candidate patched_candidate = candidate;
                patched_candidate.set_component(1);
                const webrtc::JsepIceCandidate ice_candidate{std::string(),0, patched_candidate};
                (void) strong_signaling->ice_candidate_callback_(models::IceCandidate(&ice_candidate));
            });
        });

        transport_channel->SetCandidatePairChangeCallback([weak](webrtc::CandidatePairChangeEvent const &event) {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->candidate_pair_changed(event);
        });

        transport_channel->SubscribeNetworkRouteChanged(this, [weak](const std::optional<webrtc::NetworkRoute> &route) {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            assert(strong->networkThread().IsCurrent());
            if (route.has_value()) {
                RTC_LOG(LS_VERBOSE) << "NativeNetworkingImpl route changed: " << route->DebugString();
                const bool local_is_wifi = route->local.adapter_type() == webrtc::AdapterType::ADAPTER_TYPE_WIFI;
                const bool remote_is_wifi = route->remote.adapter_type() == webrtc::AdapterType::ADAPTER_TYPE_WIFI;
                RTC_LOG(LS_VERBOSE) << "NativeNetworkingImpl is wifi: local=" << local_is_wifi << ", remote=" << remote_is_wifi;
                const std::string local_description = route->local.uses_turn() ? "turn" : "p2p";
                const std::string remote_description = route->remote.uses_turn() ? "turn" : "p2p";
                if (models::RouteDescription route_description(local_description, remote_description); !strong->current_route_description_ || route_description != strong->current_route_description_.value()) {
                    strong->current_route_description_ = std::move(route_description);
                    strong->notify_state_updated();
                }
            }
        });
    }

    std::optional<webrtc::SSLRole> NativeConnection::dtls_role() const {
        return std::nullopt;
    }

    bool NativeConnection::supports_renomination() const {
        return local_parameters_.supports_renomination;
    }

    void NativeConnection::state_updated(const bool is_connected) {
        if (!is_connected) {
            last_disconnected_timestamp_ = webrtc::TimeMillis();
        }
        notify_state_updated();
    }

    int NativeConnection::candidate_pool_size() const {
        return 0;
    }

    bool NativeConnection::get_custom_parameter_bool(const std::string& name) const {
        if (custom_parameters_.is_null()) {
            return false;
        }
        return custom_parameters_[name].is_boolean() && custom_parameters_[name];
    }

    models::CandidateDescription NativeConnection::connection_description_from_candidate(const webrtc::Candidate& candidate) {
        models::CandidateDescription result;
        result.type = candidate.type_name();
        result.protocol = candidate.protocol();
        result.address = candidate.address().ToString();
        return result;
    }

    void NativeConnection::create_channels() {
        const auto coordinated_state = content_negotiation_context_->coordinated_state();
        if (!coordinated_state) {
            return;
        }
        if (audio_channel_id_) {
            if (const auto audio_ssrc = content_negotiation_context_->outgoing_channel_ssrc(*audio_channel_id_)) {
                if (audio_channel_ && audio_channel_->ssrc() != audio_ssrc.value()) {
                    audio_channel_ = nullptr;
                }
                std::optional<models::MediaContent> audio_content;
                for (const auto &content : coordinated_state->outgoing_contents) {
                    if (content.type == models::MediaContent::Type::Audio && content.ssrc == audio_ssrc.value()) {
                        audio_content = content;
                        break;
                    }
                }
                if (audio_content) {
                    if (!audio_channel_) {
                        audio_channel_ = std::make_unique<media::channels::OutgoingAudioChannel>(
                            call_.get(),
                            channel_manager_.get(),
                            dtls_srtp_transport_.get(),
                            *audio_content,
                            worker_thread(),
                            network_thread(),
                            &audio_sink_,
                            payload_type_mapping_,
                            encryptor_,
                            nullptr
                        );
                    }
                }
            }
        }
        if (video_channel_id_) {
            if (const auto video_ssrc = content_negotiation_context_->outgoing_channel_ssrc(*video_channel_id_)) {
                if (video_channel_ && video_channel_->ssrc() != video_ssrc.value()) {
                    video_channel_ = nullptr;
                }
                std::optional<models::MediaContent> video_content;
                for (const auto &content : coordinated_state->outgoing_contents) {
                    if (content.type == models::MediaContent::Type::Video && content.ssrc == video_ssrc.value()) {
                        video_content = content;
                        break;
                    }
                }
                if (video_content) {
                    if (!video_channel_) {
                        video_channel_ = std::make_unique<media::channels::OutgoingVideoChannel>(
                            call_.get(),
                            channel_manager_.get(),
                            dtls_srtp_transport_.get(),
                            *video_content,
                            worker_thread(),
                            network_thread(),
                            &video_sink_,
                            payload_type_mapping_,
                            encryptor_
                        );
                    }
                }
            }
        }

        std::unordered_set<uint32_t> remote_channels;
        for (const auto &content : coordinated_state->incoming_contents) {
            remote_channels.insert(content.ssrc);
        }
        auto remove_channel = [&](auto &channels) {
            std::erase_if(channels, [&](const auto &entry) {
                if (const uint32_t ssrc = entry.second->ssrc(); !remote_channels.contains(ssrc)) {
                    pending_content_.erase(entry.first);
                    return true;
                }
                return false;
            });
        };
        remove_channel(incoming_audio_channels_);
        remove_channel(incoming_video_channels_);
        for (const auto &content : coordinated_state->incoming_contents) {
            add_incoming_smart_source(std::to_string(content.ssrc), content);
        }
    }

    void NativeConnection::notify_state_updated() {
        const std::weak_ptr weak(shared_from_this());
        signaling_thread().PostTask([weak] {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            ConnectionState new_value;
            if (strong->failed_) {
                new_value = ConnectionState::Failed;
            } else if (strong->connected_) {
                new_value = ConnectionState::Connected;
            } else {
                new_value = ConnectionState::Connecting;
            }
            strong->current_state_ = new_value;
            (void) strong->connection_change_callback_(new_value, strong->already_connected_);
            if (new_value == ConnectionState::Connected && !strong->already_connected_) {
                strong->already_connected_ = true;
            }
        });
    }

    void NativeConnection::candidate_pair_changed(webrtc::CandidatePairChangeEvent const& event) {
        models::ConnectionDescription connection_description;

        connection_description.local = connection_description_from_candidate(event.selected_candidate_pair.local);
        connection_description.remote = connection_description_from_candidate(event.selected_candidate_pair.remote);

        if (!current_connection_description_ || current_connection_description_.value() != connection_description) {
            current_connection_description_ = std::move(connection_description);
            notify_state_updated();
        }
    }

    void NativeConnection::start() {
        transport_channel_->MaybeStartGathering();
        data_channel_interface_ = std::make_unique<SctpDataChannelProviderInterfaceImpl>(
            environment(),
            dtls_transport_.get(),
            is_outgoing_,
            network_thread()
        );
        const std::weak_ptr weak(shared_from_this());
        data_channel_interface_->on_message_received([weak](const bytes::binary &data) {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            (void) strong->data_channel_message_callback_(data);
        });
        data_channel_interface_->on_state_changed([weak](const bool is_open) {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            if (!strong->data_channel_open_ && is_open) {
                strong->data_channel_open_ = true;
                (void) strong->data_channel_opened_callback_();
            } else {
                strong->data_channel_open_ = false;
            }
        });
        last_disconnected_timestamp_ = webrtc::TimeMillis();
        check_connection_timeout();
    }

    void NativeConnection::close() {
        NativeNetworkInterface::close();
        content_negotiation_context_ = nullptr;
        relay_port_factory_ = nullptr;
    }

    void NativeConnection::add_ice_candidate(const models::IceCandidate& raw_candidate) const {
        const bool standalone_reflector_mode = get_custom_parameter_bool("network_standalone_reflectors");
        const auto candidate = parse_ice_candidate(raw_candidate)->candidate();
        if (standalone_reflector_mode) {
            if (absl::EndsWith(candidate.address().hostname(), ".reflector")) {
                return;
            }
        }
        const std::weak_ptr weak(shared_from_this());
        network_thread().PostTask([weak, candidate] {
            const auto strong = std::static_pointer_cast<const NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->transport_channel_->AddRemoteCandidate(candidate);
        });
    }

    void NativeConnection::set_remote_params(models::PeerIceParameters remote_ice_parameters, std::unique_ptr<webrtc::SSLFingerprint> fingerprint, const std::string& ssl_setup) {
        const std::weak_ptr weak(shared_from_this());
        network_thread().PostTask([weak, remote_ice_parameters = std::move(remote_ice_parameters), fingerprint = std::move(fingerprint), ssl_setup] {
            const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
            if (!strong) {
                return;
            }
            if (strong->remote_parameters_.ufrag != remote_ice_parameters.ufrag ||
                strong->remote_parameters_.pwd != remote_ice_parameters.pwd ||
                strong->remote_parameters_.supports_renomination != remote_ice_parameters.supports_renomination) {
                strong->remote_parameters_ = remote_ice_parameters;
                const webrtc::IceParameters parameters(
                    remote_ice_parameters.ufrag,
                    remote_ice_parameters.pwd,
                    remote_ice_parameters.supports_renomination
                );
                strong->transport_channel_->SetRemoteIceParameters(parameters);
            } else {
                RTC_LOG(LS_VERBOSE) << "Remote ICE parameters unchanged, skipping";
            }
            webrtc::SSLRole ssl_role;
            if (ssl_setup == "active") {
                ssl_role = webrtc::SSLRole::SSL_SERVER;
            } else if (ssl_setup == "passive") {
                ssl_role = webrtc::SSLRole::SSL_CLIENT;
            } else {
                ssl_role = strong->is_outgoing_ ? webrtc::SSLRole::SSL_CLIENT : webrtc::SSLRole::SSL_SERVER;
            }
            if (fingerprint) {
                if (auto dtls_params = fingerprint->GetRfc4572Fingerprint() + ":" + std::to_string(static_cast<int>(ssl_role)); dtls_params != strong->applied_dtls_params_) {
                    strong->applied_dtls_params_ = std::move(dtls_params);
                    strong->dtls_transport_->SetRemoteParameters(fingerprint->algorithm, fingerprint->digest.data(), fingerprint->digest.size(), ssl_role);
                } else {
                    RTC_LOG(LS_VERBOSE) << "Remote DTLS parameters unchanged, skipping";
                }
            }
        });
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> NativeConnection::get_pending_offer() const {
        return content_negotiation_context_->get_pending_offer();
    }

    std::unique_ptr<ContentNegotiationContext::NegotiationContents> NativeConnection::set_pending_answer(std::unique_ptr<ContentNegotiationContext::NegotiationContents> answer) const {
        return content_negotiation_context_->set_pending_answer(std::move(answer));
    }

    std::unique_ptr<media::tracks::MediaTrackInterface> NativeConnection::add_outgoing_track(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) {
        if (const auto audio_track = dynamic_cast<webrtc::AudioTrackInterface*>(track.get())) {
            audio_channel_id_ = content_negotiation_context_->add_outgoing_channel(audio_track);
        }
        if (const auto video_track = dynamic_cast<webrtc::VideoTrackInterface*>(track.get())) {
            if (!video_channel_id_) {
                video_channel_id_ = content_negotiation_context_->add_outgoing_channel(video_track);
            }
        }
        return NativeNetworkInterface::add_outgoing_track(track);
    }

    bool NativeConnection::is_group_connection() const {
        return false;
    }

    void NativeConnection::check_connection_timeout() {
        const std::weak_ptr weak(shared_from_this());
        if (!closed_) {
            network_thread().PostDelayedTask([weak] {
                const auto strong = std::static_pointer_cast<NativeConnection>(weak.lock());
                if (!strong) {
                    return;
                }
                const int64_t current_timestamp = webrtc::TimeMillis();
                if (constexpr int64_t kMaxTimeout = 20000; !strong->connected_ && strong->last_disconnected_timestamp_ + kMaxTimeout < current_timestamp) {
                    RTC_LOG(LS_INFO) << "NativeNetworkingImpl timeout " << current_timestamp - strong->last_disconnected_timestamp_ << " ms";
                    strong->failed_ = true;
                    strong->notify_state_updated();
                    return;
                }
                strong->check_connection_timeout();
            }, webrtc::TimeDelta::Millis(1000));
        }
    }
} // wrtc::interfaces