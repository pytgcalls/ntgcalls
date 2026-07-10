//
// Created by Lauren on 15/03/24.
//

#pragma once
#include <memory>
#include <ntgcalls/media/stream_manager.hpp>
#include <ntgcalls/models/connection_info.hpp>
#include <ntgcalls/models/remote_source_state.hpp>
#include <ntgcalls/signaling/messages/media_state_message.hpp>
#include <wrtc/interfaces/network_interface.hpp>

namespace ntgcalls::instances {

    class CallInterface: public std::enable_shared_from_this<CallInterface> {
    protected:
        std::shared_ptr<wrtc::interfaces::NetworkInterface> connection_;
        std::shared_ptr<media::StreamManager> stream_manager_;
        wrtc::utils::synchronized_callback<void(ConnectionInfo)> connection_change_callback_;
        wrtc::utils::synchronized_callback<void(RemoteSource)> remote_source_callback_;
        wrtc::utils::SafeThread& update_thread_;
        media::StreamManager::Status last_camera_state_ = media::StreamManager::Status::Idling;
        media::StreamManager::Status last_screen_state_ = media::StreamManager::Status::Idling;
        media::StreamManager::Status last_mic_state_ = media::StreamManager::Status::Idling;

        void set_connection_observer(
            const std::shared_ptr<wrtc::interfaces::NetworkInterface>& conn,
            ConnectionInfo::Kind kind = ConnectionInfo::Kind::Normal
        );

        static media::StreamManager::Status parse_video_state(signaling::messages::MediaStateMessage::VideoState state);

    public:
        virtual ~CallInterface() = default;

        explicit CallInterface(wrtc::utils::SafeThread& update_thread);

        enum class Type {
            Group = 1 << 0,
            Outgoing = 1 << 1,
            Incoming = 1 << 2,
            P2P = Outgoing | Incoming,
            Conference = 1 << 3
        };

        virtual void stop();

        wrtc::ConnectionMode get_connection_mode() const;

        bool pause() const;

        bool resume() const;

        bool mute() const;

        bool unmute() const;

        virtual void set_stream_sources(media::StreamManager::Mode mode, const media::MediaDescription& config) const;

        void on_stream_end(const std::function<void(media::StreamManager::Type, media::StreamManager::Device)> &callback) const;

        void on_connection_change(const std::function<void(ConnectionInfo)> &callback);

        void on_frames(const std::function<void(media::StreamManager::Mode, media::StreamManager::Device, const std::vector<wrtc::models::Frame>&)>& callback) const;

        void on_remote_source_change(const std::function<void(RemoteSource)>& callback);

        uint64_t time(media::StreamManager::Mode mode) const;

        media::MediaState get_state() const;

        media::StreamManager::Status status(media::StreamManager::Mode mode) const;

        virtual Type type() const = 0;

        void send_external_frame(media::StreamManager::Device device, const bytes::binary& data, wrtc::models::FrameData frame_data) const;

        template<typename DestCallType, typename BaseCallType>
        static DestCallType* safe(const std::shared_ptr<BaseCallType>& call) {
            if (!call) {
                throw std::runtime_error("Null pointer exception");
            }
            if (auto* derived_call = dynamic_cast<DestCallType*>(call.get())) {
                return derived_call;
            }
            throw std::runtime_error("Invalid NetworkInterface type");
        }

        std::shared_ptr<media::StreamManager> stream_manager() const;
    };

    inline int operator&(const CallInterface::Type& lhs, const CallInterface::Type rhs){
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }

    inline CallInterface::Type operator|(const CallInterface::Type lhs, const CallInterface::Type rhs){
        return static_cast<CallInterface::Type>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }
} // ntgcalls::instances
