//
// Created by Laky64 on 15/03/2024.
//

#include "call_interface.hpp"

namespace ntgcalls {
    CallInterface::CallInterface(rtc::Thread* workerThread): workerThread(workerThread){
        stream = std::make_unique<Stream>(workerThread);
    }

    CallInterface::~CallInterface() {
        RTC_LOG(LS_VERBOSE) << "Destroying CallInterface";
        onStreamEnd(nullptr);
        onConnectionChange(nullptr);
        stream = nullptr;
        if (connection) {
            connection->onConnectionChange(nullptr);
            connection->close();
            RTC_LOG(LS_VERBOSE) << "Connection closed";
        }
        connection = nullptr;
        workerThread = nullptr;
        RTC_LOG(LS_VERBOSE) << "CallInterface destroyed";
    }

    bool CallInterface::pause() const {
        return stream->pause();
    }

    bool CallInterface::resume() const {
        return stream->resume();
    }

    bool CallInterface::mute() const {
        return stream->mute();
    }

    bool CallInterface::unmute() const {
        return stream->unmute();
    }

    void CallInterface::changeStream(const MediaDescription& config) const {
        stream->setAVStream(config);
    }

    void CallInterface::onStreamEnd(const std::function<void(Stream::Type)>& callback) {
        std::lock_guard lock(mutex);
        stream->onStreamEnd(callback);
    }

    void CallInterface::onConnectionChange(const std::function<void(ConnectionState)>& callback) {
        std::lock_guard lock(mutex);
        connectionChangeCallback = callback;
    }

    uint64_t CallInterface::time() const {
        return stream->time();
    }

    MediaState CallInterface::getState() const {
        return stream->getState();
    }

    Stream::Status CallInterface::status() const {
        return stream->status();
    }

    void CallInterface::setConnectionObserver() {
        RTC_LOG(LS_INFO) << "Connecting...";
        (void) connectionChangeCallback(ConnectionState::Connecting);
        connection->onConnectionChange([this](const wrtc::ConnectionState state) {
            switch (state) {
            case wrtc::ConnectionState::Connecting:
                if (connected) {
                    RTC_LOG(LS_INFO) << "Reconnecting...";
                }
                break;
            case wrtc::ConnectionState::Connected:
                RTC_LOG(LS_INFO) << "Connection established";
                if (!connected) {
                    connected = true;
                    stream->start();
                    RTC_LOG(LS_INFO) << "Stream started";
                    (void) connectionChangeCallback(ConnectionState::Connected);
                }
                break;
            case wrtc::ConnectionState::Disconnected:
            case wrtc::ConnectionState::Failed:
            case wrtc::ConnectionState::Closed:
                workerThread->PostTask([this] {
                    connection->onConnectionChange(nullptr);
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
        });
        workerThread->PostDelayedTask([this] {
            if (!workerThread) {
                return;
            }
            if (!connected) {
                RTC_LOG(LS_ERROR) << "Connection timeout";
                (void) connectionChangeCallback(ConnectionState::Timeout);
            }
        }, webrtc::TimeDelta::Seconds(20));
    }
} // ntgcalls