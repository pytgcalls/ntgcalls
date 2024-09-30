//
// Created by Laky64 on 15/03/2024.
//

#include <ntgcalls/instances/call_interface.hpp>

namespace ntgcalls {
    CallInterface::CallInterface(rtc::Thread* updateThread): updateThread(updateThread) {
        networkThread = rtc::Thread::Create();
        networkThread->Start();
        streamManager = std::make_unique<StreamManager>(updateThread);
    }

    CallInterface::~CallInterface() {
        RTC_LOG(LS_VERBOSE) << "Destroying CallInterface";
        isExiting = true;
        std::lock_guard lock(mutex);
        connectionChangeCallback = nullptr;
        streamManager = nullptr;
        if (connection) {
            connection->onConnectionChange(nullptr);
            connection = nullptr;
            RTC_LOG(LS_VERBOSE) << "Connection closed";
        }
        updateThread = nullptr;
        cancelNetworkListener();
        RTC_LOG(LS_VERBOSE) << "CallInterface destroyed";
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
    }

    void CallInterface::onStreamEnd(const std::function<void(StreamManager::Type, StreamManager::Device)>& callback) {
        std::lock_guard lock(mutex);
        streamManager->onStreamEnd(callback);
    }

    void CallInterface::onConnectionChange(const std::function<void(ConnectionState)>& callback) {
        std::lock_guard lock(mutex);
        connectionChangeCallback = callback;
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

    void CallInterface::cancelNetworkListener() {
        if (networkThread) {
            networkThread->Stop();
            networkThread = nullptr;
        }
    }

    void CallInterface::setConnectionObserver() {
        RTC_LOG(LS_INFO) << "Connecting...";
        (void) connectionChangeCallback(ConnectionState::Connecting);
        connection->onConnectionChange([this](const wrtc::ConnectionState state) {
            if (isExiting) return;
            std::lock_guard lock(mutex);
            switch (state) {
            case wrtc::ConnectionState::Connecting:
                if (connected) {
                    RTC_LOG(LS_INFO) << "Reconnecting...";
                }
                break;
            case wrtc::ConnectionState::Connected:
                RTC_LOG(LS_INFO) << "Connection established";
                if (!connected && streamManager) {
                    connected = true;
                    streamManager->start();
                    RTC_LOG(LS_INFO) << "Stream started";
                    (void) connectionChangeCallback(ConnectionState::Connected);
                }
                break;
            case wrtc::ConnectionState::Disconnected:
            case wrtc::ConnectionState::Failed:
            case wrtc::ConnectionState::Closed:
                updateThread->PostTask([this] {
                    if (connection) {
                        connection->onConnectionChange(nullptr);
                    }
                });
                if (state == wrtc::ConnectionState::Failed) {
                    RTC_LOG(LS_ERROR) << "Connection failed";
                    (void) connectionChangeCallback(ConnectionState::Failed);
                } else {
                    RTC_LOG(LS_INFO) << "Connection closed";
                    (void) connectionChangeCallback(ConnectionState::Closed);
                }
                break;
            default:
                break;
            }
            cancelNetworkListener();
        });
        networkThread->PostDelayedTask([this] {
            if (!connected) {
                RTC_LOG(LS_ERROR) << "Connection timeout";
                (void) connectionChangeCallback(ConnectionState::Timeout);
            }
        }, webrtc::TimeDelta::Seconds(20));
    }
} // ntgcalls