//
// Created by Laky64 on 15/03/2024.
//

#include <ntgcalls/instances/call_interface.hpp>

namespace ntgcalls {
    CallInterface::CallInterface(rtc::Thread* updateThread): updateThread(updateThread) {
        initNetThread();
        streamManager = std::make_shared<StreamManager>(updateThread);
    }

    void CallInterface::stop() {
        isExiting = true;
        std::weak_ptr weak(shared_from_this());
        updateThread->BlockingCall([weak] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->connectionChangeCallback = nullptr;
            strong->streamManager->close();
            strong->streamManager = nullptr;
            if (strong->connection) {
                strong->connection->close();
                strong->connection = nullptr;
            }
            strong->updateThread = nullptr;
            strong->cancelNetworkListener();
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

    void CallInterface::onConnectionChange(const std::function<void(NetworkInfo)>& callback) {
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

    void CallInterface::setConnectionObserver(const std::shared_ptr<wrtc::NetworkInterface>& conn, NetworkInfo::Kind kind) {
        RTC_LOG(LS_INFO) << "Connecting...";
        (void) connectionChangeCallback({NetworkInfo::ConnectionState::Connecting, kind});
        std::weak_ptr weak(shared_from_this());
        conn->onConnectionChange([weak, kind, conn](const wrtc::ConnectionState state, bool wasConnected) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->updateThread->PostTask([weak, kind, conn, state, wasConnected] {
                const auto strongUpdate = weak.lock();
                if (!strongUpdate) {
                    return;
                }
                if (strongUpdate->isExiting) return;
                switch (state) {
                case wrtc::ConnectionState::Connecting:
                    if (wasConnected) {
                        RTC_LOG(LS_INFO) << "Reconnecting...";
                    }
                    return;
                case wrtc::ConnectionState::Connected:
                    RTC_LOG(LS_INFO) << "Connection established";
                    if (!wasConnected && strongUpdate->streamManager) {
                        strongUpdate->streamManager->start();
                        RTC_LOG(LS_INFO) << "Stream started";
                        (void) strongUpdate->connectionChangeCallback({NetworkInfo::ConnectionState::Connected, kind});
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
                        (void) strongUpdate->connectionChangeCallback({NetworkInfo::ConnectionState::Failed, kind});
                    } else {
                        RTC_LOG(LS_INFO) << "Connection closed";
                        (void) strongUpdate->connectionChangeCallback({NetworkInfo::ConnectionState::Closed, kind});
                    }
                    break;
                default:
                    break;
                }
                strongUpdate->cancelNetworkListener();
            });
        });
        networkThread->PostDelayedTask([weak, kind, conn] {
            const auto strongNetwork = weak.lock();
            if (!strongNetwork) {
                return;
            }
            if (!conn->isConnected()) {
                RTC_LOG(LS_ERROR) << "Connection timeout";
                (void) strongNetwork->connectionChangeCallback({NetworkInfo::ConnectionState::Timeout, kind});
            }
        }, webrtc::TimeDelta::Seconds(10));
    }

    StreamManager::Status CallInterface::parseVideoState(const signaling::MediaStateMessage::VideoState state) {
        switch (state){
        case signaling::MediaStateMessage::VideoState::Active:
            return StreamManager::Status::Active;
        case signaling::MediaStateMessage::VideoState::Inactive:
            return StreamManager::Status::Idling;
        case signaling::MediaStateMessage::VideoState::Suspended:
            return StreamManager::Status::Paused;
        }
        return StreamManager::Status::Idling;
    }

    void CallInterface::initNetThread() {
        networkThread = rtc::Thread::Create();
        networkThread->Start();
    }
} // ntgcalls