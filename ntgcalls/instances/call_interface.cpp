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
        onDisconnect(nullptr);
        stream = nullptr;
        if (connection) {
            connection->onConnectionChange(nullptr);
            connection->close();
            RTC_LOG(LS_VERBOSE) << "Connection closed";
        }
        connection = nullptr;
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

    void CallInterface::onDisconnect(const std::function<void()>& callback) {
        std::lock_guard lock(mutex);
        onCloseConnection = callback;
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
} // ntgcalls