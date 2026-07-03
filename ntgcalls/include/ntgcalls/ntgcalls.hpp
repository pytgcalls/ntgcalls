//
// Created by Lauren on 22/08/23.
//
#pragma once

#include <ntgcalls/e2e/subchain_request.hpp>
#include <ntgcalls/instances/call_interface.hpp>
#include <ntgcalls/media/devices/media_devices.hpp>
#include <ntgcalls/models/remote_source_state.hpp>
#include <ntgcalls/p2p/auth_params.hpp>
#include <ntgcalls/p2p/conference_join_params.hpp>
#include <ntgcalls/p2p/dh_config.hpp>
#include <ntgcalls/p2p/protocol.hpp>
#include <ntgcalls/p2p/rtc_server.hpp>
#include <ntgcalls/utils/hardware_info.hpp>
#include <ntgcalls/utils/log_sink_impl.hpp>
#include <ntgcalls/utils/shutdown_hook.hpp>
#include <wrtc/models/media_content.hpp>
#include <wrtc/models/segment_part_request.hpp>
#include <wrtc/models/ssrc_mapping.hpp>

#define CHECK_AND_THROW_IF_EXISTS(chat_id) \
if (exists(chat_id)) { \
throw ConnectionError("Connection cannot be initialized more than once."); \
}

#define THROW_CONNECTION_NOT_FOUND(chat_id) \
throw ConnectionNotFound("Connection with chat id \"" + std::to_string(chat_id) + "\" not found");

#define WORKER_NO_LOG(worker, ...) \
worker->PostTask([__VA_ARGS__] {\

#define END_WORKER_NO_LOG });

#define WORKER(funcName, worker, ...) \
RTC_LOG(LS_VERBOSE) << funcName << ": " << "Starting worker"; \
WORKER_NO_LOG(worker, __VA_ARGS__) \
RTC_LOG(LS_VERBOSE) << funcName << ": " << "Worker started";

#define END_WORKER \
RTC_LOG(LS_VERBOSE) << "Worker finished";\
END_WORKER_NO_LOG

namespace ntgcalls {

    class NTgCalls {
        std::unordered_map<int64_t, std::shared_ptr<instances::CallInterface>> connections_;
        wrtc::utils::synchronized_callback<void(int64_t)> request_participants_callback_;
        wrtc::utils::synchronized_callback<void(int64_t, bytes::binary)> outbound_block_callback_;
        wrtc::utils::synchronized_callback<void(int64_t, e2e::SubchainRequest)> subchain_request_callback_;
        wrtc::utils::synchronized_callback<void(int64_t, media::StreamManager::Type, media::StreamManager::Device)> on_eof_callback_;
        wrtc::utils::synchronized_callback<void(int64_t, media::MediaState)> media_state_callback_;
        wrtc::utils::synchronized_callback<void(int64_t, std::string)> update_emojis_callback_;
        wrtc::utils::synchronized_callback<void(int64_t, ConnectionInfo)> connection_change_callback_;
        wrtc::utils::synchronized_callback<void(int64_t, bytes::binary)> emit_callback_;
        wrtc::utils::synchronized_callback<void(int64_t, RemoteSource)> remote_source_callback_;
        wrtc::utils::synchronized_callback<void(int64_t)> broadcast_timestamp_callback_;
        wrtc::utils::synchronized_callback<void(int64_t, wrtc::models::SegmentPartRequest)> request_broadcast_part_callback_;
        wrtc::utils::synchronized_callback<void(int64_t, media::StreamManager::Mode, media::StreamManager::Device, std::vector<wrtc::models::Frame>)> frames_callback_;
        std::unique_ptr<wrtc::utils::SafeThread> update_thread_;
        std::unique_ptr<utils::HardwareInfo> hardware_info_;
        std::mutex mutex_;
        uint64_t shutdown_token_ = 0;

        bool exists(int64_t chat_id) const;

        instances::CallInterface* safe_connection(int64_t chat_id);

        void setup_listeners(int64_t chat_id);

        template<typename DestCallType, typename BaseCallType>
        static DestCallType* safe_call(BaseCallType* call);

        void remove(int64_t chat_id);

        void stop_connections();

    public:
        explicit NTgCalls();

        ~NTgCalls();

        [[ntg::async]] void create_p2p_call(int64_t user_id);

        [[ntg::async]] bytes::binary init_exchange(int64_t user_id, const p2p::DhConfig& dh_config, const std::optional<bytes::binary> &ga_hash);

        [[ntg::async]] p2p::AuthParams exchange_keys(int64_t user_id, const bytes::binary &ga_or_gb, int64_t fingerprint);

        [[ntg::async]] void skip_exchange(int64_t user_id, const bytes::binary &encryption_key, bool is_outgoing);

        [[ntg::async]] void connect_p2p(int64_t user_id, const std::vector<p2p::RTCServer>& servers, const std::vector<std::string>& versions, bool p2p_allowed, const std::optional<std::string>& custom_parameters);

        [[ntg::async]] std::string create_call(int64_t chat_id);

        [[ntg::async]] std::string init_presentation(int64_t chat_id);

        [[ntg::async]] p2p::ConferenceJoinParams init_conference(int64_t chat_id, int64_t user_id, const std::optional<bytes::binary>& last_block);

        [[ntg::async]] void connect(int64_t chat_id, const std::string& params, bool is_presentation);

        [[ntg::async]] uint32_t add_incoming_video(int64_t chat_id, int64_t user_id, const std::string& endpoint, const std::vector<wrtc::models::SsrcGroup>& ssrc_groups);

        [[ntg::async]] bool remove_incoming_video(int64_t chat_id, const std::string& endpoint);

        [[ntg::async]] void set_stream_sources(int64_t chat_id, media::StreamManager::Mode mode, const media::MediaDescription& media);

        [[ntg::async]] bool pause(int64_t chat_id);

        [[ntg::async]] bool resume(int64_t chat_id);

        [[ntg::async]] bool mute(int64_t chat_id);

        [[ntg::async]] bool unmute(int64_t chat_id);

        [[ntg::async]] void stop(int64_t chat_id);

        [[ntg::async]] void stop_presentation(int64_t chat_id);

        [[ntg::async]] std::string get_emojis_fingerprint(int64_t chat_id);

        [[ntg::async]] uint64_t time(int64_t chat_id, media::StreamManager::Mode mode);

        [[ntg::async]] media::MediaState get_state(int64_t chat_id);

        [[ntg::async]] instances::CallInterface::Type get_call_type(int64_t chat_id);

        [[ntg::async]] wrtc::ConnectionMode get_connection_mode(int64_t chat_id);

        [[ntg::async]] double cpu_usage() const;

        static std::string ping();

        static media::devices::MediaDevices get_media_devices();

        static p2p::Protocol get_protocol();

#ifndef IS_ANDROID
        static void enable_glib_loop(bool enable);
#endif

        void on_upgrade(const std::function<void(int64_t, media::MediaState)>& callback);

        void on_stream_end(const std::function<void(int64_t, media::StreamManager::Type, media::StreamManager::Device)>& callback);

        void on_connection_change(const std::function<void(int64_t, ConnectionInfo)>& callback);

        void on_frames(const std::function<void(int64_t, media::StreamManager::Mode, media::StreamManager::Device, const std::vector<wrtc::models::Frame>&)>& callback);

        void on_signaling_data(const std::function<void(int64_t, const bytes::binary&)>& callback);

        void on_remote_source_change(const std::function<void(int64_t, RemoteSource)>& callback);

        void on_request_broadcast_part(const std::function<void(int64_t, wrtc::models::SegmentPartRequest)>& callback);

        void on_request_broadcast_timestamp(const std::function<void(int64_t)>& callback);

        void on_request_participants(const std::function<void(int64_t)>& callback);

        void on_outbound_block(const std::function<void(int64_t, const bytes::binary&)>& callback);

        void on_subchain_request(const std::function<void(int64_t, e2e::SubchainRequest)>& callback);

        void on_update_emojis(const std::function<void(int64_t, std::string)>& callback);

        [[ntg::async]] void send_broadcast_timestamp(int64_t chat_id, int64_t timestamp);

        [[ntg::async]] void send_broadcast_part(int64_t chat_id, int64_t segment_id, int32_t part_id, wrtc::models::MediaSegment::Part::Status status, bool quality_update, const std::optional<bytes::binary> &data);

        [[ntg::async]] void send_signaling_data(int64_t chat_id, const bytes::binary& msg_key);

        [[ntg::async]] void send_external_frame(int64_t chat_id, media::StreamManager::Device device, const bytes::binary &data, wrtc::models::FrameData frame_data);

        [[ntg::async]] void update_audio_ssrc_mappings(int64_t chat_id, const std::vector<wrtc::models::SsrcMapping>& ssrc_groups);

        [[ntg::async]] void apply_blocks(int64_t chat_id, int subchain, int next_offset, const std::vector<bytes::binary> &blocks, bool from_short_poll);

        [[ntg::async]] void finish_subchain_request(int64_t chat_id, int subchain);

        [[ntg::async]] std::map<int64_t, media::StreamManager::CallInfo> calls();
    };

} // ntgcalls
