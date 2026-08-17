//
// Created by Lauren on 01/10/24.
//

#include <ranges>
#include <p2p/base/basic_async_resolver_factory.h>
#include <p2p/base/p2p_constants.h>
#include <p2p/client/basic_port_allocator.h>
#include <pc/media_factory.h>
#include <rtc_base/crypto_random.h>
#include <rtc_base/rtc_certificate_generator.h>
#include <rtc_base/time_utils.h>
#include <wrtc/exceptions.hpp>
#include <wrtc/interfaces/native_network_interface.hpp>
#include <wrtc/interfaces/wrapped_dtls_srtp_transport.hpp>
#include <wrtc/models/outgoing_video_format.hpp>

namespace wrtc::interfaces {
    void NativeNetworkInterface::init_connection(bool supports_packet_sending) {
        const std::weak_ptr weak(shared_from_this());
        network_thread().BlockingCall([weak, supports_packet_sending] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->local_parameters_ = models::PeerIceParameters(
                webrtc::CreateRandomString(webrtc::ICE_UFRAG_LENGTH),
                webrtc::CreateRandomString(webrtc::ICE_PWD_LENGTH),
                true
            );
            strong->local_certificate_ = webrtc::RTCCertificateGenerator::GenerateCertificate(
                webrtc::KeyParams(webrtc::KT_ECDSA),
                std::nullopt
            );
            strong->underlying_socket_factory_ = strong->network_thread().socketServer();
            strong->async_resolver_factory_ = std::make_unique<webrtc::BasicAsyncDnsResolverFactory>();
            strong->dtls_srtp_transport_ = std::make_unique<WrappedDtlsSrtpTransport>(
                true,
                strong->environment().field_trials(),
                [weak](const webrtc::RtpPacketReceived& packet) {
                    const auto strong_listener = weak.lock();
                    if (!strong_listener) {
                        return;
                    }
                    strong_listener->worker_thread().PostTask([weak, packet] {
                        const auto strong_worker = weak.lock();
                        if (!strong_worker) {
                            return;
                        }
                        strong_worker->rtp_packet_received(packet);
                    });
                }
            );
            strong->dtls_srtp_transport_->SetDtlsTransports(nullptr, nullptr);
            strong->dtls_srtp_transport_->SubscribeReadyToSend(strong.get(), [weak](const bool ready_to_send) {
                const auto strong_listener = weak.lock();
                if (!strong_listener) {
                    return;
                }
                strong_listener->dtls_ready_to_send(ready_to_send);
            });
            strong->dtls_srtp_transport_->SubscribeRtcpPacketReceived(strong.get(), [weak](const webrtc::CopyOnWriteBuffer& packet, std::optional<webrtc::Timestamp> time, webrtc::EcnMarking) {
                const auto strong_listener = weak.lock();
                if (!strong_listener) {
                    return;
                }
                strong_listener->worker_thread().PostTask([weak, packet] {
                    const auto strong_worker = weak.lock();
                    if (!strong_worker) {
                        return;
                    }
                    if (strong_worker->call_) strong_worker->call_->Receiver()->DeliverRtcpPacket(packet);
                });
            });
            if (supports_packet_sending) {
                strong->dtls_srtp_transport_->SubscribeSentPacket(strong.get(), [weak](const webrtc::SentPacketInfo& packet) {
                    const auto strong_listener = weak.lock();
                    if (!strong_listener) {
                        return;
                    }
                    strong_listener->worker_thread().PostTask([weak, packet] {
                        const auto strong_worker = weak.lock();
                        if (!strong_worker) {
                            return;
                        }
                        if (strong_worker->call_) strong_worker->call_->OnSentPacket(packet);
                    });
                });
            }
            strong->reset_dtls_srtp_transport();
        });
        channel_manager_ = std::make_unique<media::ChannelManager>(
            environment(),
            factory_->media_engine(),
            worker_thread(),
            network_thread(),
            signaling_thread()
        );
        worker_thread().BlockingCall([weak] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            webrtc::CallConfig call_config(strong->environment(), strong->network_thread());
            call_config.audio_state = strong->factory_->media_engine()->voice().GetAudioState();
            strong->call_ = strong->factory_->media_factory()->CreateCall(std::move(call_config));
            strong->payload_type_suggester_ = std::make_unique<webrtc::SdpPayloadTypeSuggester>(
                webrtc::PeerConnectionInterface::kBundlePolicyMaxBundle,
                strong->environment()
            );
        });
        available_video_formats_ = filter_supported_video_formats(factory_->get_supported_video_formats());

        payload_type_mapping_.insert(std::make_pair(111, media::FrameTransformer::PayloadType::Opus));
        for (const auto temp_video_payload_types = models::OutgoingVideoFormat::assign_payload_types(available_video_formats_); const auto &it : temp_video_payload_types) {
            if (it.video_codec().name == webrtc::kVp8CodecName) {
                payload_type_mapping_.insert(std::make_pair(it.video_codec().id.value(), media::FrameTransformer::PayloadType::VP8));
            } else if (it.video_codec().name == webrtc::kH264CodecName) {
                payload_type_mapping_.insert(std::make_pair(it.video_codec().id.value(), media::FrameTransformer::PayloadType::H264));
            }
        }
    }

    void NativeNetworkInterface::add_incoming_smart_source(const std::string& endpoint, const models::MediaContent& media_content, const bool force) {
        const std::lock_guard lock(mutex_);
        if (pending_content_.contains(endpoint) && !force) {
            return;
        }
        bool is_addable = false;
        switch (media_content.type) {
        case models::MediaContent::Type::Audio:
            is_addable = audio_incoming_;
            break;
        case models::MediaContent::Type::Video:
            if (media_content.is_screen_cast()) {
                is_addable = screen_incoming_;
            } else {
                is_addable = camera_incoming_;
            }
            break;
        }
        if (is_addable && media_content.type == models::MediaContent::Type::Audio) {
            if (incoming_audio_channels_.size() > 10) {
                int64_t min_activity = INT64_MAX;
                const auto timestamp = webrtc::TimeMillis();
                std::string min_activity_channel_id;
                for (const auto& [channelId, channel] : incoming_audio_channels_) {
                    if (const auto activity = channel->get_activity(); activity < min_activity && activity < timestamp - 1000) {
                        min_activity = activity;
                        min_activity_channel_id = channelId;
                    }
                }
                if (!min_activity_channel_id.empty()) {
                    remove_incoming_audio(min_activity_channel_id);
                }
                if (incoming_audio_channels_.size() > 10) {
                    RTC_LOG(LS_WARNING) << "Too many incoming audio channels, unable to add " << endpoint << " ssrc";
                    return;
                }
            }
            RTC_LOG(LS_INFO) << "Adding incoming audio channel with ssrc " << media_content.main_ssrc();
            if (const auto sink = remote_audio_sink_.lock()) sink->add_source();
            incoming_audio_channels_[endpoint] = std::make_unique<media::channels::IncomingAudioChannel>(
                call_.get(),
                channel_manager_.get(),
                dtls_srtp_transport_.get(),
                media_content,
                worker_thread(),
                network_thread(),
                remote_audio_sink_,
                payload_type_mapping_,
                encryptor_,
                nullptr
            );
        } else if (is_addable && media_content.type == models::MediaContent::Type::Video) {
            auto video_codecs = models::OutgoingVideoFormat::get_video_codecs(
                available_video_formats_,
                media_content.payload_types,
                is_group_connection()
            );
            incoming_video_channels_[endpoint] = std::make_unique<media::channels::IncomingVideoChannel>(
                call_.get(),
                channel_manager_.get(),
                dtls_srtp_transport_.get(),
                media_content.ssrc_groups,
                factory_->ssrc_generator(),
                video_codecs,
                worker_thread(),
                network_thread(),
                media_content.is_screen_cast() ? remote_screen_cast_sink_ : remote_video_sink_,
                payload_type_mapping_,
                encryptor_
            );
        }
        if (pending_content_.contains(endpoint)) {
            return;
        }
        int audio_channels_count = 0;
        for (const auto& content : pending_content_ | std::views::values) {
            if (content.type == models::MediaContent::Type::Audio) {
                audio_channels_count++;
            }
        }
        if (audio_channels_count >= 10) {
            return;
        }
        pending_content_[endpoint] = media_content;
    }

    void NativeNetworkInterface::remove_incoming_audio(const std::string& endpoint) {
        if (!pending_content_.contains(endpoint)) {
            return;
        }
        RTC_LOG(LS_INFO) << "Removing incoming audio channel with ssrc " << endpoint;
        if (incoming_audio_channels_.contains(endpoint)) incoming_audio_channels_.erase(endpoint);
        pending_content_.erase(endpoint);
        if (const auto sink = remote_audio_sink_.lock()) sink->remove_source();
    }

    void NativeNetworkInterface::dtls_ready_to_send(const bool is_ready_to_send) {
        update_aggregate_states_n();

        if (is_ready_to_send) {
            const std::weak_ptr weak(shared_from_this());
            network_thread().PostTask([weak] {
                const auto strong = weak.lock();
                if (!strong) {
                    return;
                }
                strong->update_aggregate_states_n();
            });
        }
    }

    void NativeNetworkInterface::update_aggregate_states_n() {
        const auto state = transport_channel_->GetIceTransportState();
        bool is_connected = false;
        switch (state) {
        case webrtc::IceTransportState::kConnected:
        case webrtc::IceTransportState::kCompleted:
            is_connected = true;
            break;
        default:
            break;
        }
        if (!dtls_srtp_transport_->IsWritable(false)) {
            is_connected = false;
        }
        if (connected_ != is_connected) {
            connected_ = is_connected;
            state_updated(is_connected);
            if (data_channel_interface_) {
                data_channel_interface_->update_is_connected(is_connected);
            }
        }
    }

    void NativeNetworkInterface::reset_dtls_srtp_transport() {
        port_allocator_ = std::make_unique<webrtc::BasicPortAllocator>(
            environment(),
            factory_->network_manager(),
            factory_->socket_factory(),
            nullptr,
            get_relay_port_factory()
        );
        set_port_allocator_flags(port_allocator_.get());
        port_allocator_->Initialize();
        auto [stunServers, turnServers] = get_stun_and_turn_servers();
        port_allocator_->SetConfiguration(stunServers, turnServers, candidate_pool_size(), webrtc::NO_PRUNE);

        webrtc::IceTransportInit ice_transport_init(environment());
        ice_transport_init.set_port_allocator(port_allocator_.get());
        ice_transport_init.set_async_dns_resolver_factory(async_resolver_factory_.get());
        transport_channel_ = webrtc::P2PTransportChannel::Create("transport", 0, std::move(ice_transport_init));

        webrtc::IceConfig ice_config;
        ice_config.continual_gathering_policy = webrtc::GATHER_CONTINUALLY;
        ice_config.prioritize_most_likely_candidate_pairs = true;
        ice_config.regather_on_failed_networks_interval = get_regather_on_failed_networks_interval();
        if (get_custom_parameter_bool("network_skip_initial_ping")) {
            ice_config.presume_writable_when_fully_relayed = true;
        }
        transport_channel_->SetIceConfig(ice_config);

        const std::weak_ptr weak(shared_from_this());
        const webrtc::IceParameters local_ice_parameters(
            local_parameters_.ufrag,
            local_parameters_.pwd,
            supports_renomination()
        );
        transport_channel_->SetIceParameters(local_ice_parameters);
        transport_channel_->SetIceRole(ice_role());
        transport_channel_->SubscribeRoleConflict(this, [weak](webrtc::IceTransportInternal*) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->handle_role_conflict();
        });
        transport_channel_->SubscribeIceTransportStateChanged(this, [weak](webrtc::IceTransportInternal*) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->update_aggregate_states_n();
        });
        register_transport_callbacks(transport_channel_.get());

        dtls_transport_ = std::make_unique<webrtc::DtlsTransportInternalImpl>(environment(), transport_channel_.get(), get_default_crypto_options());
        dtls_transport_->SubscribeReceivingState(this, [weak](webrtc::PacketTransportInternal*) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            assert(strong->networkThread().IsCurrent());
            strong->update_aggregate_states_n();
        });
        dtls_transport_->SubscribeWritableState(this,[weak](webrtc::PacketTransportInternal*) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            assert(strong->networkThread().IsCurrent());
            strong->update_aggregate_states_n();
        });
        if (const auto role = dtls_role(); role.has_value()) {
            dtls_transport_->SetDtlsRole(role.value());
        }
        dtls_transport_->SetLocalCertificate(local_certificate_);
        dtls_srtp_transport_->SetDtlsTransports(dtls_transport_.get(), nullptr);
    }

    void NativeNetworkInterface::handle_role_conflict() {
        assert(network_thread().IsCurrent());
        if (!transport_channel_ || is_group_connection()) {
            return;
        }
        const auto reversed_role = transport_channel_->GetIceRole() == webrtc::ICEROLE_CONTROLLING
            ? webrtc::ICEROLE_CONTROLLED
            : webrtc::ICEROLE_CONTROLLING;
        RTC_LOG(LS_WARNING) << "Got ICE role conflict, switching to "
            << (reversed_role == webrtc::ICEROLE_CONTROLLING ? "controlling" : "controlled") << " role";
        transport_channel_->SetIceRole(reversed_role);
    }

    webrtc::CryptoOptions NativeNetworkInterface::get_default_crypto_options() {
        auto options = webrtc::CryptoOptions();
        options.srtp.enable_aes128_sha1_80_crypto_cipher = true;
        options.srtp.enable_gcm_crypto_suites = true;
        return options;
    }

    std::vector<std::string> NativeNetworkInterface::get_endpoints() const {
        std::vector<std::string> endpoints;
        for (const auto &[endpoint, media] : pending_content_) {
            if (media.type == models::MediaContent::Type::Video) {
                endpoints.push_back(endpoint);
            }
        }
        return endpoints;
    }

    ConnectionMode NativeNetworkInterface::get_connection_mode() const {
        return ConnectionMode::Rtc;
    }

    void NativeNetworkInterface::enable_audio_incoming(const bool enable) {
        if (audio_incoming_ == enable) {
            return;
        }
        NetworkInterface::enable_audio_incoming(enable);

        const std::weak_ptr weak(shared_from_this());
        worker_thread().BlockingCall([weak, enable] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            if (enable) {
                for (const auto& [endpoint, mediaContent] : strong->pending_content_) {
                    if (mediaContent.type == models::MediaContent::Type::Audio) {
                        strong->add_incoming_smart_source(endpoint, mediaContent, true);
                    }
                }
            } else {
                const std::lock_guard lock(strong->mutex_);
                strong->incoming_audio_channels_.clear();
            }
        });
    }

    void NativeNetworkInterface::enable_video_incoming(const bool enable, const bool is_screen_cast) {
        if (is_screen_cast) {
            if (screen_incoming_ == enable) {
                return;
            }
        } else {
            if (camera_incoming_ == enable) {
                return;
            }
        }
        NetworkInterface::enable_video_incoming(enable, is_screen_cast);
        const std::weak_ptr weak(shared_from_this());
        worker_thread().BlockingCall([weak, enable, is_screen_cast] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            if (enable) {
                for (const auto& [endpoint, mediaContent] : strong->pending_content_) {
                    if (mediaContent.type == models::MediaContent::Type::Video && mediaContent.is_screen_cast() == is_screen_cast) {
                        strong->add_incoming_smart_source(endpoint, mediaContent, true);
                    }
                }
            } else {
                for (const auto& [endpoint, mediaContent] : strong->pending_content_) {
                    if (mediaContent.type == models::MediaContent::Type::Video && mediaContent.is_screen_cast() == is_screen_cast) {
                        strong->incoming_video_channels_.erase(endpoint);
                    }
                }
            }
        });
    }

    std::unique_ptr<webrtc::SSLFingerprint> NativeNetworkInterface::local_fingerprint() const {
        const auto certificate = local_certificate_;
        if (!certificate) {
            return nullptr;
        }
        return webrtc::SSLFingerprint::CreateFromCertificate(*certificate);
    }

    void NativeNetworkInterface::close() {
        const std::weak_ptr weak(shared_from_this());
        worker_thread().BlockingCall([weak] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->pending_content_.clear();
            strong->audio_channel_ = nullptr;
            strong->video_channel_ = nullptr;
            strong->incoming_audio_channels_.clear();
            strong->incoming_video_channels_.clear();
            strong->remote_audio_sink_.reset();
            strong->remote_video_sink_.reset();
            strong->remote_screen_cast_sink_.reset();
            strong->call_ = nullptr;
        });
        channel_manager_ = nullptr;
        if (factory_) {
            RTC_LOG(LS_VERBOSE) << "Removed call";
            network_thread().BlockingCall([weak] {
                const auto strong = weak.lock();
                if (!strong) {
                    return;
                }
                if (strong->transport_channel_) {
                    strong->transport_channel_->UnsubscribeNetworkRouteChanged(strong.get());
                }
                strong->data_channel_interface_ = nullptr;
                if (strong->dtls_transport_) {
                    strong->dtls_transport_->UnsubscribeWritableState(strong.get());
                }
                if (strong->dtls_srtp_transport_) {
                    strong->dtls_srtp_transport_->UnsubscribeSentPacket(strong.get());
                    strong->dtls_srtp_transport_->SetDtlsTransports(nullptr, nullptr);
                }
                strong->dtls_srtp_transport_ = nullptr;
                strong->dtls_transport_ = nullptr;
                strong->transport_channel_ = nullptr;
                strong->port_allocator_ = nullptr;
                strong->underlying_socket_factory_ = nullptr;
                strong->async_resolver_factory_ = nullptr;
                strong->local_certificate_ = nullptr;
            });
            signaling_thread().BlockingCall([] {});
        }
        NetworkInterface::close();
        encryptor_ = nullptr;
    }

    void NativeNetworkInterface::add_incoming_audio_track(const std::weak_ptr<media::RemoteAudioSink>& sink) {
        remote_audio_sink_ = sink;
    }

    void NativeNetworkInterface::add_incoming_video_track(const std::weak_ptr<media::RemoteVideoSink>& sink, const bool is_screen_cast) {
        if (is_screen_cast) {
            remote_screen_cast_sink_ = sink;
        } else {
            remote_video_sink_ = sink;
        }
    }

    std::unique_ptr<media::tracks::MediaTrackInterface> NativeNetworkInterface::add_outgoing_track(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) {
        const std::weak_ptr weak(shared_from_this());
        if (const auto audio_track = dynamic_cast<webrtc::AudioTrackInterface*>(track.get())) {
            audio_track->AddSink(&audio_sink_);
            return std::make_unique<media::tracks::MediaTrackInterface>([weak](const bool enable) {
                const auto strong = weak.lock();
                if (!strong) {
                    return;
                }
                if (strong->audio_channel_ != nullptr) {
                    strong->audio_channel_->set_enabled(enable);
                }
            });
        }
        if (const auto video_track = dynamic_cast<webrtc::VideoTrackInterface*>(track.get())) {
            video_track->AddOrUpdateSink(&video_sink_, webrtc::VideoSinkWants());
            return std::make_unique<media::tracks::MediaTrackInterface>([weak](const bool enable) {
                const auto strong = weak.lock();
                if (!strong) {
                    return;
                }
                if (strong->video_channel_ != nullptr) {
                    strong->video_channel_->set_enabled(enable);
                }
            });
        }
        throw RTCException("Unsupported track type");
    }

    models::PeerIceParameters NativeNetworkInterface::local_ice_parameters() {
        return local_parameters_;
    }

    void NativeNetworkInterface::send_data_channel_message(const bytes::binary& data) const {
        const std::weak_ptr weak(shared_from_this());
        network_thread().PostTask([weak, data] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            if (strong->data_channel_interface_) {
                strong->data_channel_interface_->send_data_channel_message(data);
            }
        });
    }

    std::vector<webrtc::SdpVideoFormat> NativeNetworkInterface::filter_supported_video_formats(std::vector<webrtc::SdpVideoFormat> const& formats) {
        std::vector<webrtc::SdpVideoFormat> filtered_formats;

        std::vector<std::string> filter_codec_names = {
            webrtc::kVp8CodecName,
            webrtc::kVp9CodecName,
            webrtc::kH264CodecName
        };

        std::vector<webrtc::SdpVideoFormat> vp9_formats;
        std::vector<webrtc::SdpVideoFormat> h264_formats;

        for (const auto &format : formats) {
            if (std::ranges::find(filter_codec_names, format.name) == filter_codec_names.end()) {
                continue;
            }

            if (format.name == webrtc::kVp9CodecName) {
                vp9_formats.push_back(format);
            } else if (format.name == webrtc::kH264CodecName) {
                h264_formats.push_back(format);
            } else {
                filtered_formats.push_back(format);
            }
        }

        if (!vp9_formats.empty()) {
            bool added = false;
            for (const auto &format : vp9_formats) {
                if (added) {
                    break;
                }
                for (const auto & [fst, snd] : format.parameters) {
                    if (fst == "profile-id") {
                        if (snd == "0") {
                            filtered_formats.push_back(format);
                            added = true;
                            break;
                        }
                    }
                }
            }

            if (!added) {
                filtered_formats.push_back(vp9_formats[0]);
            }
        }

        if (!h264_formats.empty()) {
            std::ranges::sort(h264_formats, [](const webrtc::SdpVideoFormat &lhs, const webrtc::SdpVideoFormat &rhs) {
                auto [lProfileLevelId, lPacketizationMode, lLevelAssymetryAllowed] = parse_h264_format_parameters(lhs);
                auto [rProfileLevelId, rPacketizationMode, rLevelAssymetryAllowed] = parse_h264_format_parameters(rhs);

                const int lhs_level_id_priority = get_h264_profile_level_id_priority(lProfileLevelId);
                const int lhs_packetization_mode_priority = get_h264_packetization_mode_priority(lPacketizationMode);
                const int lhs_level_assymetry_allowed_priority = get_h264_level_assymetry_allowed_priority(lLevelAssymetryAllowed);

                const int rhs_level_id_priority = get_h264_profile_level_id_priority(rProfileLevelId);
                const int rhs_packetization_mode_priority = get_h264_packetization_mode_priority(rPacketizationMode);
                const int rhs_level_assymetry_allowed_priority = get_h264_level_assymetry_allowed_priority(rLevelAssymetryAllowed);

                if (lhs_level_id_priority != rhs_level_id_priority) {
                    return lhs_level_id_priority < rhs_level_id_priority;
                }
                if (lhs_packetization_mode_priority != rhs_packetization_mode_priority) {
                    return lhs_packetization_mode_priority < rhs_packetization_mode_priority;
                }
                if (lhs_level_assymetry_allowed_priority != rhs_level_assymetry_allowed_priority) {
                    return lhs_level_assymetry_allowed_priority < rhs_level_assymetry_allowed_priority;
                }

                return false;
            });

            filtered_formats.push_back(h264_formats[0]);
        }

        return filtered_formats;
    }

    NativeNetworkInterface::H264FormatParameters NativeNetworkInterface::parse_h264_format_parameters(webrtc::SdpVideoFormat const& format) {
        H264FormatParameters result;
        for (const auto & [fst, snd] : format.parameters) {
            if (fst == "profile-level-id") {
                result.profile_level_id = snd;
            } else if (fst == "packetization-mode") {
                result.packetization_mode = snd;
            } else if (fst == "level-asymmetry-allowed") {
                result.level_assymetry_allowed = snd;
            }
        }
        return result;
    }

    int NativeNetworkInterface::get_h264_profile_level_id_priority(const std::string& profile_level_id) {
        if (profile_level_id == webrtc::kH264ProfileLevelConstrainedHigh) {
            return 0;
        }
        if (profile_level_id == webrtc::kH264ProfileLevelConstrainedBaseline) {
            return 1;
        }
        return 2;
    }

    int NativeNetworkInterface::get_h264_packetization_mode_priority(const std::string& packetization_mode) {
        if (packetization_mode == "1") {
            return 0;
        }
        return 1;
    }

    int NativeNetworkInterface::get_h264_level_assymetry_allowed_priority(const std::string& level_assymetry_allowed) {
        if (level_assymetry_allowed == "1") {
            return 0;
        }
        return 1;
    }
} // wrtc::interfaces