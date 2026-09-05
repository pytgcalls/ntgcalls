//
// Created by Lauren on 22/08/23.
//

#include <ntgcalls/ntgcalls.hpp>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/devices/media_device.hpp>
#include <ntgcalls/instances/group_call.hpp>
#include <ntgcalls/instances/p2p_call.hpp>
#include <ntgcalls/instances/conference_call.hpp>
#include <ntgcalls/p2p/dh_config.hpp>
#include <ntgcalls/utils/g_lib_loop_manager.hpp>
#include <wrtc/video_factory/video_factory_config.hpp>

namespace ntgcalls {
    NTgCalls::NTgCalls() {
        update_thread_ = wrtc::utils::SafeThread::Create();
        update_thread_->Start();
        hardware_info_ = std::make_unique<utils::HardwareInfo>();
        utils::LogSink::get_or_create();
        shutdown_token_ = utils::ShutdownHook::add([this] {
            stop_connections();
        });
    }

    NTgCalls::~NTgCalls() {
        utils::ShutdownHook::remove(shutdown_token_);
        {
            const std::lock_guard lock(mutex_);
            RTC_LOG(LS_VERBOSE) << "Destroying NTgCalls";
            on_eof_callback_ = nullptr;
            media_state_callback_ = nullptr;
            connection_change_callback_ = nullptr;
            emit_callback_ = nullptr;
            remote_source_callback_ = nullptr;
            broadcast_timestamp_callback_ = nullptr;
            request_broadcast_part_callback_ = nullptr;
            frames_callback_ = nullptr;
            hardware_info_ = nullptr;
        }
        stop_connections();
        update_thread_->Stop();
        update_thread_ = nullptr;
        RTC_LOG(LS_VERBOSE) << "NTgCalls destroyed";
        utils::LogSink::un_ref();
    }

    void NTgCalls::stop_connections() {
        decltype(connections_) local_connections;
        {
            const std::lock_guard lock(mutex_);
            local_connections = std::move(connections_);
        }
        for (const auto& connection : local_connections | std::views::values) {
            connection->stop();
        }
    }

    void NTgCalls::setup_listeners(const int64_t chat_id) {
        connections_[chat_id]->on_stream_end([this, chat_id](const media::StreamManager::Type& type, const media::StreamManager::Device& device) {
            WORKER("onStreamEnd", update_thread_, this, chat_id, type, device)
            (void) on_eof_callback_(chat_id, type, device);
            END_WORKER
        });
        if (connections_[chat_id]->type() & (instances::CallInterface::Type::Group | instances::CallInterface::Type::Conference)) {
            safe_call<instances::GroupCall>(connections_[chat_id].get())->on_upgrade([this, chat_id](const media::MediaState& state) {
                WORKER("onUpgrade", update_thread_, this, chat_id, state)
                (void) media_state_callback_(chat_id, state);
                END_WORKER
            });

            safe_call<instances::GroupCall>(connections_[chat_id].get())->on_request_broadcast_part([this, chat_id](const wrtc::models::SegmentPartRequest& request) {
                WORKER_NO_LOG(update_thread_, this, chat_id, request)
                (void) request_broadcast_part_callback_(chat_id, request);
                END_WORKER_NO_LOG
            });

            safe_call<instances::GroupCall>(connections_[chat_id].get())->on_request_broadcast_timestamp([this, chat_id] {
                WORKER_NO_LOG(update_thread_, this, chat_id)
                (void) broadcast_timestamp_callback_(chat_id);
                END_WORKER_NO_LOG
            });
            if (connections_[chat_id]->type() & instances::CallInterface::Type::Conference) {
                safe_call<instances::ConferenceCall>(connections_[chat_id].get())->on_request_participants([this, chat_id] {
                    WORKER("onRequestParticipants", update_thread_, this, chat_id)
                    (void) request_participants_callback_(chat_id);
                    END_WORKER
                });
                safe_call<instances::ConferenceCall>(connections_[chat_id].get())->on_outbound_block([this, chat_id](const bytes::binary& block) {
                    WORKER("onOutboundBlock", update_thread_, this, chat_id, block)
                    (void) outbound_block_callback_(chat_id, block);
                    END_WORKER
                });
                safe_call<instances::ConferenceCall>(connections_[chat_id].get())->on_subchain_request([this, chat_id](e2e::SubchainRequest subchain_request) {
                    WORKER("onSubchainRequest", update_thread_, this, chat_id, subchain_request)
                    (void) subchain_request_callback_(chat_id, subchain_request);
                    END_WORKER
                });
            }
        }
        if (connections_[chat_id]->type() & (instances::CallInterface::Type::P2P | instances::CallInterface::Type::Conference)) {
            safe_call<instances::E2EInterface>(connections_[chat_id].get())->on_update_emojis([this, chat_id](const std::string& emojis) {
                WORKER("onUpdateEmojis", update_thread_, this, chat_id, emojis)
                (void) update_emojis_callback_(chat_id, emojis);
                END_WORKER
            });
        }
        connections_[chat_id]->on_connection_change([this, chat_id](const ConnectionInfo& state) {
            WORKER("onConnectionChange", update_thread_, this, chat_id, state)
            (void) connection_change_callback_(chat_id, state);
            if (state.kind == ConnectionInfo::Kind::Normal) {
                switch (state.state) {
                case ConnectionInfo::State::Closed:
                case ConnectionInfo::State::Failed:
                case ConnectionInfo::State::Timeout:
                    update_thread_->PostTask([this, chat_id] {
                        remove(chat_id);
                    });
                    break;
                default:
                    break;
                }
            }
            END_WORKER
        });
        connections_[chat_id]->on_frames([this, chat_id](const media::StreamManager::Mode mode, const media::StreamManager::Device device, const std::vector<wrtc::models::Frame>& frames) {
            (void) frames_callback_(chat_id, mode, device, frames);
        });
        connections_[chat_id]->on_remote_source_change([this, chat_id](const RemoteSource& state) {
            WORKER("onRemoteSourceChange", update_thread_, this, chat_id, state)
            (void) remote_source_callback_(chat_id, state);
            END_WORKER
        });
        if (connections_[chat_id]->type() & instances::CallInterface::Type::P2P) {
            safe_call<instances::P2PCall>(connections_[chat_id].get())->on_signaling_data([this, chat_id](const bytes::binary& data) {
                WORKER("onSignalingData", update_thread_, this, chat_id, data)
                (void) emit_callback_(chat_id, data);
                END_WORKER
            });
        }
    }

    void NTgCalls::create_p2p_call(const int64_t user_id) {
        const std::lock_guard lock(mutex_);
        CHECK_AND_THROW_IF_EXISTS(user_id)
        connections_[user_id] = std::make_shared<instances::P2PCall>(*update_thread_);
        setup_listeners(user_id);
        safe_call<instances::P2PCall>(connections_[user_id].get())->init();
    }

    bytes::binary NTgCalls::init_exchange(const int64_t user_id, const p2p::DhConfig& dh_config, const std::optional<bytes::binary>& ga_hash) {
        return safe_call<instances::P2PCall>(safe_connection(user_id))->init_exchange(dh_config, ga_hash);
    }

    p2p::AuthParams NTgCalls::exchange_keys(const int64_t user_id, const bytes::binary& g_a_or_b, const int64_t fingerprint) {
        return safe_call<instances::P2PCall>(safe_connection(user_id))->exchange_keys(g_a_or_b, fingerprint);
    }

    void NTgCalls::skip_exchange(const int64_t user_id, const bytes::binary& encryption_key, const bool is_outgoing) {
        safe_call<instances::P2PCall>(safe_connection(user_id))->skip_exchange(encryption_key, is_outgoing);
    }

    void NTgCalls::connect_p2p(const int64_t user_id, const std::vector<p2p::RTCServer>& servers, const std::vector<std::string>& versions, const bool p2p_allowed, const std::optional<std::string>& custom_parameters) {
        safe_call<instances::P2PCall>(safe_connection(user_id))->connect(servers, versions, p2p_allowed, custom_parameters);
    }

    std::string NTgCalls::create_call(const int64_t chat_id) {
        const std::lock_guard lock(mutex_);
        CHECK_AND_THROW_IF_EXISTS(chat_id)
        connections_[chat_id] = std::make_shared<instances::GroupCall>(*update_thread_);
        setup_listeners(chat_id);
        return safe_call<instances::GroupCall>(connections_[chat_id].get())->init();
    }

    std::string NTgCalls::init_presentation(const int64_t chat_id) {
        return safe_call<instances::GroupCall>(safe_connection(chat_id))->init_presentation();
    }

    p2p::ConferenceJoinParams NTgCalls::init_conference(const int64_t chat_id, const int64_t user_id, const std::optional<bytes::binary>& last_block) {
        const std::lock_guard lock(mutex_);
        if (!exists(chat_id)) {
            THROW_CONNECTION_NOT_FOUND(chat_id)
        }
        auto conference_call = std::make_shared<instances::ConferenceCall>(*update_thread_);
        if (auto* p2p_call = safe_call<instances::P2PCall>(connections_[chat_id].get())) {
            RTC_LOG(LS_INFO) << "Migrating P2P call to conference call for " << chat_id;
            conference_call->migrate(p2p_call);
            p2p_call->stop();
        }
        connections_[chat_id] = std::move(conference_call);
        auto result = safe_call<instances::ConferenceCall>(connections_[chat_id].get())->init_conference(user_id, last_block);
        setup_listeners(chat_id);
        return result;
    }

    void NTgCalls::connect(const int64_t chat_id, const std::string& params, const bool is_presentation) {
        safe_call<instances::GroupCall>(safe_connection(chat_id))->connect(params, is_presentation);
    }

    uint32_t NTgCalls::add_incoming_video(const int64_t chat_id, const int64_t user_id, const std::string& endpoint, const std::vector<wrtc::models::SsrcGroup>& ssrc_groups) {
        return safe_call<instances::GroupCall>(safe_connection(chat_id))->add_incoming_video(user_id, endpoint, ssrc_groups);
    }

    bool NTgCalls::remove_incoming_video(const int64_t chat_id, const std::string& endpoint) {
        return safe_call<instances::GroupCall>(safe_connection(chat_id))->remove_incoming_video(endpoint);
    }

    void NTgCalls::update_audio_ssrc_mappings(const int64_t chat_id, const std::vector<wrtc::models::SsrcMapping>& ssrc_groups) {
        return safe_call<instances::ConferenceCall>(safe_connection(chat_id))->update_audio_ssrc_mappings(ssrc_groups);
    }

    void NTgCalls::apply_blocks(const int64_t chat_id, const int subchain, const int next_offset, const std::vector<bytes::binary>& blocks, const bool from_short_poll) {
        return safe_call<instances::ConferenceCall>(safe_connection(chat_id))->apply_blocks(subchain, next_offset, blocks, from_short_poll);
    }

    void NTgCalls::finish_subchain_request(const int64_t chat_id, const int subchain) {
        return safe_call<instances::ConferenceCall>(safe_connection(chat_id))->finish_subchain_request(subchain);
    }

    void NTgCalls::set_stream_sources(const int64_t chat_id, const media::StreamManager::Mode mode, const media::MediaDescription& media) {
        safe_connection(chat_id)->set_stream_sources(mode, media);
    }

    bool NTgCalls::pause(const int64_t chat_id) {
        return safe_connection(chat_id)->pause();
    }

    bool NTgCalls::resume(const int64_t chat_id) {
        return safe_connection(chat_id)->resume();
    }

    bool NTgCalls::mute(const int64_t chat_id) {
        return safe_connection(chat_id)->mute();
    }

    bool NTgCalls::unmute(const int64_t chat_id) {
        return safe_connection(chat_id)->unmute();
    }

    void NTgCalls::stop(const int64_t chat_id) {
        remove(chat_id);
    }

    void NTgCalls::stop_presentation(const int64_t chat_id) {
        safe_call<instances::GroupCall>(safe_connection(chat_id))->stop_presentation(true);
    }

    std::string NTgCalls::get_emojis_fingerprint(const int64_t chat_id) {
        return safe_call<instances::E2EInterface>(safe_connection(chat_id))->get_fingerprint_emojis();
    }

    void NTgCalls::on_stream_end(const std::function<void(int64_t, media::StreamManager::Type, media::StreamManager::Device)>& callback) {
        const std::lock_guard lock(mutex_);
        on_eof_callback_ = callback;
    }

    void NTgCalls::on_upgrade(const std::function<void(int64_t, media::MediaState)>& callback) {
        const std::lock_guard lock(mutex_);
        media_state_callback_ = callback;
    }

    void NTgCalls::on_connection_change(const std::function<void(int64_t, ConnectionInfo)>& callback) {
        const std::lock_guard lock(mutex_);
        connection_change_callback_ = callback;
    }

    void NTgCalls::on_frames(const std::function<void(int64_t, media::StreamManager::Mode, media::StreamManager::Device, const std::vector<wrtc::models::Frame>&)>& callback) {
        const std::lock_guard lock(mutex_);
        frames_callback_ = callback;
    }

    void NTgCalls::on_signaling_data(const std::function<void(int64_t, const bytes::binary&)>& callback) {
        const std::lock_guard lock(mutex_);
        emit_callback_ = callback;
    }

    void NTgCalls::on_remote_source_change(const std::function<void(int64_t, RemoteSource)>& callback) {
        const std::lock_guard lock(mutex_);
        remote_source_callback_ = callback;
    }

    void NTgCalls::on_request_broadcast_part(const std::function<void(int64_t, wrtc::models::SegmentPartRequest)>& callback) {
        const std::lock_guard lock(mutex_);
        request_broadcast_part_callback_ = callback;
    }

    void NTgCalls::on_request_broadcast_timestamp(const std::function<void(int64_t)>& callback) {
        const std::lock_guard lock(mutex_);
        broadcast_timestamp_callback_ = callback;
    }

    void NTgCalls::on_request_participants(const std::function<void(int64_t)>& callback) {
        const std::lock_guard lock(mutex_);
        request_participants_callback_ = callback;
    }

    void NTgCalls::on_outbound_block(const std::function<void(int64_t, const bytes::binary&)>& callback) {
        const std::lock_guard lock(mutex_);
        outbound_block_callback_ = callback;
    }

    void NTgCalls::on_subchain_request(const std::function<void(int64_t, e2e::SubchainRequest)>& callback) {
        const std::lock_guard lock(mutex_);
        subchain_request_callback_ = callback;
    }

    void NTgCalls::on_update_emojis(const std::function<void(int64_t, std::string)>& callback) {
        const std::lock_guard lock(mutex_);
        update_emojis_callback_ = callback;
    }

    void NTgCalls::send_broadcast_timestamp(const int64_t chat_id, const int64_t timestamp) {
        safe_call<instances::GroupCall>(safe_connection(chat_id))->send_broadcast_timestamp(timestamp);
    }

    void NTgCalls::send_broadcast_part(const int64_t chat_id, const int64_t segment_id, const int32_t part_id, const wrtc::models::MediaSegment::Part::Status status, const bool quality_update, const std::optional<bytes::binary>& data) {
        safe_call<instances::GroupCall>(safe_connection(chat_id))->send_broadcast_part(segment_id, part_id, status, quality_update, data);
    }

    void NTgCalls::send_signaling_data(const int64_t chat_id, const bytes::binary& msg_key) {
        safe_call<instances::P2PCall>(safe_connection(chat_id))->send_signaling_data(msg_key);
    }

    void NTgCalls::send_external_frame(const int64_t chat_id, const media::StreamManager::Device device, const bytes::binary& data, const wrtc::models::FrameData frame_data) {
        safe_connection(chat_id)->send_external_frame(device, data, frame_data);
    }

    uint64_t NTgCalls::time(const int64_t chat_id, const media::StreamManager::Mode mode) {
        return safe_connection(chat_id)->time(mode);
    }

    media::MediaState NTgCalls::get_state(const int64_t chat_id) {
        return safe_connection(chat_id)->get_state();
    }

    instances::CallInterface::Type NTgCalls::get_call_type(const int64_t chat_id) {
        const auto type = safe_connection(chat_id)->type();
        if (type & instances::CallInterface::Type::Conference) {
            return instances::CallInterface::Type::Conference;
        }
        if (type & instances::CallInterface::Type::Group) {
            return instances::CallInterface::Type::Group;
        }
        return instances::CallInterface::Type::P2P;
    }

    wrtc::ConnectionMode NTgCalls::get_connection_mode(const int64_t chat_id) {
        return safe_connection(chat_id)->get_connection_mode();
    }

    double NTgCalls::cpu_usage() const {
        return hardware_info_->get_cpu_usage();
    }

    std::map<int64_t, media::StreamManager::CallInfo> NTgCalls::calls() {
        std::unordered_map<int64_t, std::shared_ptr<instances::CallInterface>> connections;
        {
            const std::lock_guard lock(mutex_);
            connections = connections_;
        }
        std::map<int64_t, media::StreamManager::CallInfo> status_list;
        for (const auto& [fst, snd] : connections) {
            status_list.emplace(
                fst,
                media::StreamManager::CallInfo{
                    .playback = snd->status(media::StreamManager::Mode::Playback),
                    .capture = snd->status(media::StreamManager::Mode::Capture),
                }
            );
        }
        return status_list;
    }

    void NTgCalls::remove(const int64_t chat_id) {
        RTC_LOG(LS_VERBOSE) << "Removing call " << chat_id << ", Acquiring lock";
        std::shared_ptr<instances::CallInterface> call;
        {
            const std::lock_guard lock(mutex_);
            RTC_LOG(LS_VERBOSE) << "Lock acquired, removing call " << chat_id;
            if (!exists(chat_id)) {
                RTC_LOG(LS_WARNING) << "Call " << chat_id << " not found, already removed";
                return;
            }
            call = std::move(connections_[chat_id]);
            connections_.erase(chat_id);
        }
        call->stop();
        RTC_LOG(LS_VERBOSE) << "Call " << chat_id << " removed";
    }

    bool NTgCalls::exists(const int64_t chat_id) const {
        return connections_.contains(chat_id);
    }

    instances::CallInterface* NTgCalls::safe_connection(const int64_t chat_id) {
        const std::lock_guard lock(mutex_);
        if (!exists(chat_id)) {
            THROW_CONNECTION_NOT_FOUND(chat_id)
        }
        return connections_[chat_id].get();
    }

    p2p::Protocol NTgCalls::get_protocol() {
        return {
            92,
            92,
            true,
            true,
            signaling::Signaling::supported_versions(),
        };
    }

#ifndef IS_ANDROID
    void NTgCalls::enable_glib_loop(const bool enable) {
        utils::GLibLoopManager::enable_event_loop(enable);
    }
#endif

    template<typename DestCallType, typename BaseCallType>
    DestCallType* NTgCalls::safe_call(BaseCallType* call) {
        if (!call) {
            return nullptr;
        }
        if (auto* derived_call = dynamic_cast<DestCallType*>(call)) {
            return derived_call;
        }
        throw ConnectionError("Invalid call type");
    }

    std::string NTgCalls::ping() {
        return "pong";
    }

    media::devices::MediaDevices NTgCalls::get_media_devices() {
        const auto devices = media::devices::MediaDevice::get_audio_devices();
        std::vector<media::devices::DeviceInfo> microphones, speakers;
        for (const auto& device : devices) {
            if (wrtc::utils::json::parse(device.metadata)["is_microphone"]) {
                microphones.emplace_back(device.name, device.metadata);
            } else {
                speakers.emplace_back(device.name, device.metadata);
            }
        }
        return {
            microphones,
            speakers,
            media::devices::MediaDevice::get_camera_devices(),
            media::devices::MediaDevice::get_screen_devices()
        };
    }
} // ntgcalls
