//
// Created by Lauren on 01/10/24.
//

#include <random>
#include <modules/rtp_rtcp/source/rtp_header_extensions.h>
#include <p2p/base/dtls_transport.h>
#include <p2p/client/basic_port_allocator.h>
#include <rtc_base/time_utils.h>
#include <wrtc/exceptions.hpp>
#include <wrtc/interfaces/group_connection.hpp>
#include <wrtc/models/simulcast_layer.hpp>

namespace wrtc::interfaces {
    GroupConnection::GroupConnection(const bool is_presentation, const bool is_conference):
    is_presentation_(is_presentation),
    is_conference_(is_conference) {}

    void GroupConnection::open() {
        init_connection(true);
        generate_ssrcs();
        begin_audio_channel_cleanup_timer();
    }

    void GroupConnection::generate_ssrcs() {
        auto generator = std::mt19937(std::random_device()());
        auto distribution = std::uniform_int_distribution<uint32_t>();
        do {
            outgoing_audio_ssrc_ = distribution(generator) & 0x7fffffffU;
        } while (!outgoing_audio_ssrc_);
        outgoing_video_ssrc_ = outgoing_audio_ssrc_ + 1;
        int num_video_simulcast_layers = 3;
        if (is_conference_) {
            num_video_simulcast_layers = 1;
        } else if (is_presentation_) {
            num_video_simulcast_layers = 2;
        }
        std::vector<models::SimulcastLayer> outgoing_video_ssrcs;
        outgoing_video_ssrcs.reserve(num_video_simulcast_layers);
        for (int layer_index = 0; layer_index < num_video_simulcast_layers; layer_index++) {
            outgoing_video_ssrcs.emplace_back(outgoing_video_ssrc_ + layer_index * 2 + 0, outgoing_video_ssrc_ + layer_index * 2 + 1);
        }
        std::vector<uint32_t> simulcast_group_ssrcs;
        std::vector<webrtc::SsrcGroup> fid_groups;
        for (const auto &layer : outgoing_video_ssrcs) {
            simulcast_group_ssrcs.push_back(layer.ssrc);
            const webrtc::SsrcGroup fid_group(webrtc::kFidSsrcGroupSemantics, { layer.ssrc, layer.fid_ssrc });
            fid_groups.push_back(fid_group);
        }

        if (simulcast_group_ssrcs.size() > 1) {
            models::SsrcGroup simulcast_group;
            simulcast_group.semantics = "SIM";
            simulcast_group.ssrcs = simulcast_group_ssrcs;
            outgoing_video_ssrc_groups_.push_back(simulcast_group);
        }

        for (const auto& fid_group : fid_groups) {
            models::SsrcGroup payload_fid_group;
            payload_fid_group.semantics = "FID";
            payload_fid_group.ssrcs = fid_group.ssrcs;
            outgoing_video_ssrc_groups_.push_back(payload_fid_group);
        }
    }

    void GroupConnection::state_updated(const bool is_connected) {
        if (is_rtc_connected_ == is_connected) {
            return;
        }
        is_rtc_connected_ = is_connected;
        update_is_connected();
    }

    int GroupConnection::candidate_pool_size() const {
        return 2;
    }

    void GroupConnection::set_port_allocator_flags(webrtc::BasicPortAllocator* port_allocator) {
        uint32_t flags = port_allocator->flags();
        flags |=
            webrtc::PORTALLOCATOR_ENABLE_IPV6 |
            webrtc::PORTALLOCATOR_ENABLE_IPV6_ON_WIFI;
        port_allocator->set_flags(flags);
    }

    void GroupConnection::start() {
        transport_channel_->MaybeStartGathering();
        restart_data_channel();
    }

    void GroupConnection::restart_data_channel() {
        data_channel_interface_ = std::make_unique<SctpDataChannelProviderInterfaceImpl>(
            environment(),
            dtls_transport_.get(),
            true,
            network_thread()
        );

        const std::weak_ptr weak(shared_from_this());
        data_channel_interface_->on_message_received([weak](const bytes::binary &data) {
            const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
            if (!strong) {
                return;
            }
           (void) strong->data_channel_message_callback_(data);
        });

        data_channel_interface_->on_state_changed([weak](const bool is_open) {
            const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
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

        data_channel_interface_->update_is_connected(connected_);
    }

    std::string GroupConnection::get_join_payload() {
        RTC_LOG(LS_VERBOSE) << "Asking for join payload";
        const std::weak_ptr weak(shared_from_this());
        return network_thread().BlockingCall([weak] {
            const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
            if (!strong) {
                return std::string();
            }
            RTC_LOG(LS_VERBOSE) << "Generating join payload";
            const auto fingerprint = strong->local_fingerprint();
            utils::json json_res = {
                {"ufrag", strong->local_parameters_.ufrag},
                {"pwd", strong->local_parameters_.pwd},
                {"fingerprints",
                    {
                        {
                            {"hash", fingerprint->algorithm},
                            {"setup", "passive"},
                            {"fingerprint", fingerprint->GetRfc4572Fingerprint()}
                        }
                    }
                },
                {"ssrc", *reinterpret_cast<const int32_t*>(&strong->outgoing_audio_ssrc_)},
                {"ssrc-groups", utils::json::array()}
            };
            for (const auto& [semantics, sources] : strong->outgoing_video_ssrc_groups_) {
                std::vector<int32_t> signed_sources;
                signed_sources.reserve(sources.size());
                for (const auto source : sources) {
                    signed_sources.push_back(*reinterpret_cast<const int32_t *>(&source));
                }
                json_res["ssrc-groups"].push_back({
                    {"sources", signed_sources},
                    {"semantics", semantics}
                });
            }
            RTC_LOG(LS_VERBOSE) << "Join payload generated";
            return json_res.dump();
        });
    }

    void GroupConnection::add_ice_candidate(const models::IceCandidate& raw_candidate) const {
        const auto candidate = parse_ice_candidate(raw_candidate)->candidate();
        const std::weak_ptr weak(shared_from_this());
        network_thread().PostTask([weak, candidate] {
            const auto strong = std::static_pointer_cast<const GroupConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->transport_channel_->AddRemoteCandidate(candidate);
        });
    }

    void GroupConnection::set_remote_params(models::PeerIceParameters remote_ice_parameters, std::unique_ptr<webrtc::SSLFingerprint> fingerprint) {
        const std::weak_ptr weak(shared_from_this());
        network_thread().PostTask([weak, remote_ice_parameters = std::move(remote_ice_parameters), fingerprint = std::move(fingerprint)] {
            const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->remote_parameters_ = remote_ice_parameters;
            const webrtc::IceParameters parameters(
                remote_ice_parameters.ufrag,
                remote_ice_parameters.pwd,
                true
            );
            strong->transport_channel_->SetRemoteIceParameters(parameters);
            if (fingerprint) {
                strong->dtls_transport_->SetRemoteParameters(fingerprint->algorithm, fingerprint->digest.data(), fingerprint->digest.size(), std::nullopt);
            }
        });
    }

    void GroupConnection::connect_media_stream() {
        if (!mtproto_stream_) {
            throw RTCException("MTProto stream not initialized");
        }
        mtproto_stream_->connect();

        const std::weak_ptr weak(shared_from_this());
        network_thread().PostDelayedTask([weak] {
            const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->is_stream_connected_ = true;
            strong->update_is_connected();
        }, webrtc::TimeDelta::Millis(500));
    }

    void GroupConnection::set_connection_mode(const ConnectionMode kind) {
        connection_mode_ = kind;
        const std::weak_ptr weak(shared_from_this());
        switch (kind) {
        case ConnectionMode::Rtc:
            if (mtproto_stream_) {
                RTC_LOG(LS_INFO) << "Migrating to RTC connection";
                mtproto_stream_->close();
                mtproto_stream_ = nullptr;
                already_connected_ = false;
                if (const auto audio_sink = remote_audio_sink_.lock()) {
                    audio_sink->update_audio_source_count(0);
                }
                remote_screen_cast_sink_.reset();
            }
            network_thread().PostTask([weak] {
                const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
                if (!strong) {
                    return;
                }
                strong->start();
            });
            break;
        case ConnectionMode::Stream:
        case ConnectionMode::Rtmp:
            mtproto_stream_ = std::make_shared<mtproto::MTProtoStream>(signaling_thread(), connection_mode_ == ConnectionMode::Rtmp);
            mtproto_stream_->on_audio_frame([weak](std::unique_ptr<models::AudioFrame> frame) {
                const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
                if (!strong) {
                    return;
                }
                if (const auto audio_sink = strong->remote_audio_sink_.lock()) {
                    audio_sink->send_data(std::move(frame));
                }
            });
            mtproto_stream_->on_video_frame([weak](const uint32_t ssrc, const bool is_presentation, std::unique_ptr<webrtc::VideoFrame> frame) {
                const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
                if (!strong) {
                    return;
                }
                if (is_presentation) {
                    if (const auto video_sink = strong->remote_screen_cast_sink_.lock()) {
                        video_sink->send_frame(ssrc, std::move(frame));
                    }
                } else {
                    if (const auto video_sink = strong->remote_video_sink_.lock()) {
                        video_sink->send_frame(ssrc, std::move(frame));
                    }
                }
            });
            mtproto_stream_->on_update_audio_source_count([weak](const int count) {
                const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
                if (!strong) {
                    return;
                }
                if (const auto audio_sink = strong->remote_audio_sink_.lock()) {
                    audio_sink->update_audio_source_count(count);
                }
            });
            break;
        default:
            throw RTCException("Invalid connection mode");
        }
        update_is_connected();
    }

    void GroupConnection::send_broadcast_part(const int64_t segment_id, const int32_t part_id, const models::MediaSegment::Part::Status status, const bool quality_update, const std::optional<bytes::binary>& data) const {
        if (mtproto_stream_) {
            mtproto_stream_->send_broadcast_part(segment_id, part_id, status, quality_update, data);
        } else {
            throw RTCException("MTProto stream not initialized");
        }
    }

    void GroupConnection::on_request_broadcast_part(const std::function<void(models::SegmentPartRequest)>& callback) const {
        if (mtproto_stream_) {
            mtproto_stream_->on_request_broadcast_part(callback);
        } else {
            throw RTCException("MTProto stream not initialized");
        }
    }

    void GroupConnection::send_broadcast_timestamp(const int64_t timestamp) const {
        if (mtproto_stream_) {
            mtproto_stream_->send_broadcast_timestamp(timestamp);
        } else {
            throw RTCException("MTProto stream not initialized");
        }
    }

    void GroupConnection::on_request_broadcast_timestamp(const std::function<void()>& callback) const {
        if (mtproto_stream_) {
            mtproto_stream_->on_request_broadcast_time(callback);
        } else {
            throw RTCException("MTProto stream not initialized");
        }
    }

    void GroupConnection::update_is_connected() {
        bool is_effectively_connected = false;
        switch (connection_mode_) {
            case ConnectionMode::Rtc:
                is_effectively_connected = is_rtc_connected_;
                break;
            case ConnectionMode::Stream:
            case ConnectionMode::Rtmp:
                is_effectively_connected = is_stream_connected_;
                break;
            default:
                break;
        }
        if (is_effectively_connected != last_effectively_connected_) {
            last_effectively_connected_ = is_effectively_connected;
            const std::weak_ptr weak(shared_from_this());
            signaling_thread().PostTask([weak, new_value = is_effectively_connected ? ConnectionState::Connected : ConnectionState::Connecting] {
                const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
                if (!strong) {
                    return;
                }
                strong->current_state_ = new_value;
                (void) strong->connection_change_callback_(new_value, strong->already_connected_);
                if (new_value == ConnectionState::Connected && !strong->already_connected_) {
                    strong->already_connected_ = true;
                }
            });
        }
    }

    void GroupConnection::rtp_packet_received(const webrtc::RtpPacketReceived& packet) {
        if (is_presentation_) {
            // TODO: Support for system audio
            return;
        }
        const std::string endpoint = std::to_string(packet.Ssrc());
        if (packet.HasExtension(webrtc::kRtpExtensionAudioLevel)) {
            webrtc::AudioLevel audio_level;
            if (packet.GetExtension<webrtc::AudioLevelExtension>(&audio_level)) {
                if (incoming_audio_channels_.contains(endpoint)) incoming_audio_channels_[endpoint]->update_activity();
            }
        }

        if (packet.PayloadType() != 111) {
            return;
        }

        if (const auto it = incoming_audio_channels_.find(endpoint); it != incoming_audio_channels_.end()) {
            it->second->update_activity();
            return;
        }

        if (!is_conference_) {
            add_incoming_audio(0, packet.Ssrc(), endpoint);
            return;
        }

        if (const auto it = audio_ssrc_to_user_id_.find(packet.Ssrc());it != audio_ssrc_to_user_id_.end()) {
            add_incoming_audio(it->second, packet.Ssrc(), endpoint);
            return;
        }

        if (pending_audio_ssrcs_.insert(packet.Ssrc()).second) {
            (void) request_participants_callback_();
        }
    }

    void GroupConnection::create_channels(const ResponsePayload::Media& media) {
        media_config_ = media;
        if (audio_channel_ && audio_channel_->ssrc() != outgoing_audio_ssrc_) {
            audio_channel_ = nullptr;
        }
        models::MediaContent audio_content;
        audio_content.ssrc = outgoing_audio_ssrc_;
        audio_content.rtp_extensions = media.audio_rtp_extensions;
        audio_content.payload_types = media.audio_payload_types;

        if (!audio_channel_) {
            audio_channel_ = std::make_unique<media::channels::OutgoingAudioChannel>(
                call_.get(),
                channel_manager_.get(),
                dtls_srtp_transport_.get(),
                audio_content,
                worker_thread(),
                network_thread(),
                &audio_sink_,
                payload_type_mapping_,
                encryptor_,
                nullptr
            );
        }

        if (video_channel_ && video_channel_->ssrc() != outgoing_video_ssrc_) {
            video_channel_ = nullptr;
        }

        models::MediaContent video_content;
        video_content.ssrc = outgoing_video_ssrc_;
        video_content.ssrc_groups = outgoing_video_ssrc_groups_;
        video_content.rtp_extensions = media.video_rtp_extensions;
        video_content.payload_types = media.video_payload_types;

        if (!video_channel_) {
            video_channel_ = std::make_unique<media::channels::OutgoingVideoChannel>(
                call_.get(),
                channel_manager_.get(),
                dtls_srtp_transport_.get(),
                video_content,
                worker_thread(),
                network_thread(),
                &video_sink_,
                payload_type_mapping_,
                encryptor_
            );
        }
    }

    void GroupConnection::update_audio_ssrc_mappings(const std::vector<models::SsrcMapping> &audio_ssrcs) {
        audio_ssrc_to_user_id_.clear();
        for (const auto&[userID, ssrc] : audio_ssrcs) {
            audio_ssrc_to_user_id_[ssrc] = userID;
            if (auto endpoint = std::to_string(ssrc); pending_audio_ssrcs_.erase(ssrc) && !incoming_audio_channels_.contains(endpoint)) {
                add_incoming_audio(userID, ssrc, endpoint);
            }
        }
    }

    uint32_t GroupConnection::add_incoming_video(const int64_t user_id, const std::string& endpoint, const std::vector<models::SsrcGroup>& ssrc_groups) {
        if (pending_content_.contains(endpoint)) {
            return 0;
        }
        models::MediaContent media_content;
        media_content.type = models::MediaContent::Type::Video;
        media_content.user_id = user_id;
        media_content.ssrc_groups = ssrc_groups;
        if (mtproto_stream_) {
            mtproto_stream_->add_incoming_video(
                endpoint,
                media_content.main_ssrc(),
                media_content.is_screen_cast()
            );
        } else {
            add_incoming_smart_source(endpoint, media_content);
        }
        return media_content.main_ssrc();
    }

    bool GroupConnection::remove_incoming_video(const std::string& endpoint) {
        if (mtproto_stream_) {
            return mtproto_stream_->remove_incoming_video(endpoint);
        }
        if (!pending_content_.contains(endpoint)) {
            return false;
        }
        if (incoming_video_channels_.contains(endpoint)) incoming_video_channels_.erase(endpoint);
        pending_content_.erase(endpoint);
        return true;
    }

    void GroupConnection::on_request_participants(const std::function<void()> &callback) {
        request_participants_callback_ = callback;
    }

    void GroupConnection::set_e2e_encryptor(media::E2EEncryptor *encryptor) {
        this->encryptor_ = encryptor;
    }

    void GroupConnection::add_incoming_audio(const int64_t user_id, const uint32_t ssrc, const std::string& endpoint) {
        models::MediaContent audio_content;
        audio_content.type = models::MediaContent::Type::Audio;
        audio_content.ssrc = ssrc;
        audio_content.user_id = user_id;
        audio_content.rtp_extensions = media_config_.audio_rtp_extensions;
        audio_content.payload_types = media_config_.audio_payload_types;
        add_incoming_smart_source(endpoint, audio_content);
    }

    void GroupConnection::enable_audio_incoming(const bool enable) {
        if (mtproto_stream_) {
            mtproto_stream_->enable_audio_incoming(enable);
        } else {
            NativeNetworkInterface::enable_audio_incoming(enable);
        }
    }

    void GroupConnection::enable_video_incoming(const bool enable, const bool is_screen_cast) {
        if (mtproto_stream_) {
            mtproto_stream_->enable_video_incoming(enable, is_screen_cast);
        } else {
            NativeNetworkInterface::enable_video_incoming(enable, is_screen_cast);
        }
    }

    void GroupConnection::begin_audio_channel_cleanup_timer() {
        if (!factory_) {
            return;
        }
        const std::weak_ptr weak(shared_from_this());
        worker_thread().PostDelayedTask([weak] {
            const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
            if (!strong) {
                return;
            }
            const std::lock_guard lock(strong->mutex_);
            const auto timestamp = webrtc::TimeMillis();
            std::vector<std::string> remove_channels;
            for (const auto& [channelId, channel] : strong->incoming_audio_channels_) {
                if (channel->get_activity() < timestamp - 1000) {
                    remove_channels.push_back(channelId);
                }
            }
            for (const auto &channel_id : remove_channels) {
                strong->remove_incoming_audio(channel_id);
            }
            strong->begin_audio_channel_cleanup_timer();
        }, webrtc::TimeDelta::Millis(500));
    }

    bool GroupConnection::is_group_connection() const {
        return true;
    }

    void GroupConnection::close() {
        outgoing_video_ssrc_groups_.clear();
        if (mtproto_stream_) {
            mtproto_stream_->close();
            mtproto_stream_ = nullptr;
        }
        NativeNetworkInterface::close();
    }

    ResponsePayload::Media GroupConnection::get_media_config() const {
        return media_config_;
    }

    ConnectionMode GroupConnection::get_connection_mode() const {
        return connection_mode_;
    }

    bool GroupConnection::supports_renomination() const {
        return false;
    }

    bool GroupConnection::get_custom_parameter_bool(const std::string& name) const {
        return false;
    }

    webrtc::IceRole GroupConnection::ice_role() const {
        return webrtc::ICEROLE_CONTROLLED;
    }

    webrtc::IceMode GroupConnection::ice_mode() const {
        return webrtc::ICEMODE_FULL;
    }

    std::optional<webrtc::SSLRole> GroupConnection::dtls_role() const {
        return webrtc::SSLRole::SSL_SERVER;
    }

    std::pair<webrtc::ServerAddresses, std::vector<webrtc::RelayServerConfig>> GroupConnection::get_stun_and_turn_servers() {
        return {{}, {}};
    }

    webrtc::RelayPortFactoryInterface* GroupConnection::get_relay_port_factory() {
        return nullptr;
    }

    void GroupConnection::register_transport_callbacks(webrtc::P2PTransportChannel* transport_channel) {
        const std::weak_ptr weak(shared_from_this());
        transport_channel->RegisterReceivedPacketCallback(this, [weak](webrtc::PacketTransportInternal*, const webrtc::ReceivedIpPacket&) {
            const auto strong = std::static_pointer_cast<GroupConnection>(weak.lock());
            if (!strong) {
                return;
            }
            strong->last_network_activity_ms_ = webrtc::TimeMillis();
        });
    }

    webrtc::TimeDelta GroupConnection::get_regather_on_failed_networks_interval() {
        return webrtc::TimeDelta::Seconds(2);
    }
} // wrtc::interfaces