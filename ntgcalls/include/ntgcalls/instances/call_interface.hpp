//
// Created by Laky64 on 15/03/2024.
//

#pragma once
#include <memory>

#include <ntgcalls/stream_manager.hpp>
#include <ntgcalls/models/call_network_state.hpp>
#include <ntgcalls/models/remote_source_state.hpp>
#include <ntgcalls/signaling/messages/media_state_message.hpp>

namespace ntgcalls {

    class CallInterface {
        std::atomic_bool isExiting;

        void cancelNetworkListener();

    protected:
        std::shared_ptr<wrtc::NetworkInterface> connection;
        std::shared_ptr<StreamManager> streamManager;
        wrtc::synchronized_callback<CallNetworkState> connectionChangeCallback;
        wrtc::synchronized_callback<RemoteSource> remoteSourceCallback;
        rtc::Thread* updateThread;
        std::unique_ptr<rtc::Thread> networkThread;
        RemoteSource::State lastCameraState = RemoteSource::State::Inactive;
        RemoteSource::State lastScreenState = RemoteSource::State::Inactive;
        RemoteSource::State lastMicState = RemoteSource::State::Inactive;

        void setConnectionObserver(
            const std::shared_ptr<wrtc::NetworkInterface>& conn,
            CallNetworkState::Kind kind = CallNetworkState::Kind::Normal
        );

        static RemoteSource::State parseVideoState(signaling::MediaStateMessage::VideoState state);

        void initNetThread();

    public:
        explicit CallInterface(rtc::Thread* updateThread);

        virtual ~CallInterface();

        enum class Type {
            Group = 1 << 0,
            Outgoing = 1 << 1,
            Incoming = 1 << 2,
            P2P = Outgoing | Incoming
        };

        bool pause() const;

        bool resume() const;

        bool mute() const;

        bool unmute() const;

        virtual void setStreamSources(StreamManager::Mode mode, const MediaDescription& config) const;

        void onStreamEnd(const std::function<void(StreamManager::Type, StreamManager::Device)> &callback) const;

        void onConnectionChange(const std::function<void(CallNetworkState)> &callback);

        void onFrames(const std::function<void(StreamManager::Mode, StreamManager::Device, const std::vector<wrtc::Frame>&)>& callback) const;

        void onRemoteSourceChange(const std::function<void(RemoteSource)>& callback);

        uint64_t time(StreamManager::Mode mode) const;

        MediaState getState() const;

        StreamManager::Status status(StreamManager::Mode mode) const;

        virtual Type type() const = 0;

        void sendExternalFrame(StreamManager::Device device, const bytes::binary& data, wrtc::FrameData frameData) const;

        template<typename DestCallType, typename BaseCallType>
        static DestCallType* Safe(const std::shared_ptr<BaseCallType>& call) {
            if (!call) {
                return nullptr;
            }
            if (auto* derivedCall = dynamic_cast<DestCallType*>(call.get())) {
                return derivedCall;
            }
            throw std::runtime_error("Invalid NetworkInterface type");
        }
    };

    inline int operator&(const CallInterface::Type& lhs, const CallInterface::Type rhs){
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }
} // ntgcalls
