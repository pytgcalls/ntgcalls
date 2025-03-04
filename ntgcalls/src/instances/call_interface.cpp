//
// Created by Laky64 on 15/03/2024.
//

#include <ntgcalls/instances/call_interface.hpp>

namespace ntgcalls {
    CallInterface::CallInterface(rtc::Thread* updateThread): updateThread(updateThread) {
        initNetThread();
        streamManager = std::make_shared<StreamManager>(updateThread);
    }

    CallInterface::~CallInterface() {
        // TODO: Fixed deadlock?
        RTC_LOG(LS_VERBOSE) << "Destroying CallInterface";
        isExiting = true;
        updateThread->BlockingCall([this] {
            RTC_LOG(LS_VERBOSE) << "Destroying CallInterface";
            connectionChangeCallback = nullptr;
            RTC_LOG(LS_VERBOSE) << "Removing stream sources";
            streamManager = nullptr;
            if (connection) {
                RTC_LOG(LS_VERBOSE) << "Removing connection listener";
                connection->onConnectionChange(nullptr);
                RTC_LOG(LS_VERBOSE) << "Closing connection";
                connection->close();
                RTC_LOG(LS_VERBOSE) << "Connection closed";
                connection = nullptr;
                RTC_LOG(LS_VERBOSE) << "Connection destroyed";
            }
            updateThread = nullptr;
            cancelNetworkListener();
            RTC_LOG(LS_VERBOSE) << "CallInterface destroyed";
        });
    }

    bool CallInterface::pause() const {
        return streamManager->pause();
    }

    bool CallInterface::resume() const {
        return streamManager->resume();
    }

    bool CallInterface::mute() const {
        return streamManager->mute();
    }

    bool CallInterface::unmute() const {
        return streamManager->unmute();
    }

    void CallInterface::setStreamSources(const StreamManager::Mode mode, const MediaDescription& config) const {
        streamManager->setStreamSources(mode, config);
        if (mode == StreamManager::Mode::Playback && connection) {
            streamManager->optimizeSources(connection.get());
        }
    }

    void CallInterface::onStreamEnd(const std::function<void(StreamManager::Type, StreamManager::Device)>& callback) const {
        streamManager->onStreamEnd(callback);
    }

    void CallInterface::onConnectionChange(const std::function<void(CallNetworkState)>& callback) {
        connectionChangeCallback = callback;
    }

    void CallInterface::onFrames(const std::function<void(StreamManager::Mode, StreamManager::Device, const std::vector<wrtc::Frame>&)>& callback) const {
        streamManager->onFrames(callback);
    }

    void CallInterface::onRemoteSourceChange(const std::function<void(RemoteSource)>& callback) {
        remoteSourceCallback = callback;
    }

    uint64_t CallInterface::time(const StreamManager::Mode mode) const {
        return streamManager->time(mode);
    }

    MediaState CallInterface::getState() const {
        return streamManager->getState();
    }

    StreamManager::Status CallInterface::status(const StreamManager::Mode mode) const {
        return streamManager->status(mode);
    }

    void CallInterface::sendExternalFrame(const StreamManager::Device device, const bytes::binary& data, const wrtc::FrameData frameData) const {
        streamManager->sendExternalFrame(device, data, frameData);
    }

    void CallInterface::cancelNetworkListener() {
        if (networkThread) {
            networkThread->Stop();
            networkThread = nullptr;
        }
    }

    void CallInterface::setConnectionObserver(const std::shared_ptr<wrtc::NetworkInterface>& conn, CallNetworkState::Kind kind) {
        RTC_LOG(LS_INFO) << "Connecting...";
        (void) connectionChangeCallback({CallNetworkState::ConnectionState::Connecting, kind});
        conn->onConnectionChange([this, kind, conn](const wrtc::ConnectionState state, bool wasConnected) {
            updateThread->PostTask([this, kind, conn, state, wasConnected] {
                if (isExiting) return;
                switch (state) {
                case wrtc::ConnectionState::Connecting:
                    if (wasConnected) {
                        RTC_LOG(LS_INFO) << "Reconnecting...";
                    }
                    break;
                case wrtc::ConnectionState::Connected:
                    RTC_LOG(LS_INFO) << "Connection established";
                    if (!wasConnected && streamManager) {
                        streamManager->start();
                        RTC_LOG(LS_INFO) << "Stream started";
                        (void) connectionChangeCallback({CallNetworkState::ConnectionState::Connected, kind});
                    }
                    break;
                case wrtc::ConnectionState::Disconnected:
                case wrtc::ConnectionState::Failed:
                case wrtc::ConnectionState::Closed:
                    if (conn) {
                        conn->onConnectionChange(nullptr);
                    }
                    if (state == wrtc::ConnectionState::Failed) {
                        RTC_LOG(LS_ERROR) << "Connection failed";
                        (void) connectionChangeCallback({CallNetworkState::ConnectionState::Failed, kind});
                    } else {
                        RTC_LOG(LS_INFO) << "Connection closed";
                        (void) connectionChangeCallback({CallNetworkState::ConnectionState::Closed, kind});
                    }
                    break;
                default:
                    break;
                }
                cancelNetworkListener();
            });
        });
        networkThread->PostDelayedTask([this, kind, conn] {
            if (!conn->isConnected()) {
                RTC_LOG(LS_ERROR) << "Connection timeout";
                (void) connectionChangeCallback({CallNetworkState::ConnectionState::Timeout, kind});
            }
        }, webrtc::TimeDelta::Seconds(20));
    }

    RemoteSource::State CallInterface::parseVideoState(const signaling::MediaStateMessage::VideoState state) {
        switch (state){
        case signaling::MediaStateMessage::VideoState::Active:
            return RemoteSource::State::Active;
        case signaling::MediaStateMessage::VideoState::Inactive:
            return RemoteSource::State::Inactive;
        case signaling::MediaStateMessage::VideoState::Suspended:
            return RemoteSource::State::Suspended;
        }
        return RemoteSource::State::Inactive;
    }

    void CallInterface::initNetThread() {
        networkThread = rtc::Thread::Create();
        networkThread->Start();
    }
} // ntgcalls