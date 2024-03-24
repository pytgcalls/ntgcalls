//
// Created by Laky64 on 15/03/2024.
//

#include "call_interface.hpp"

namespace ntgcalls {
    CallInterface::CallInterface() {
        stream = std::make_unique<Stream>();
    }

    CallInterface::~CallInterface() {
        stop();
        connection = nullptr;
        stream = nullptr;
    }

    bool CallInterface::pause() {
        std::lock_guard lock(mutex);
        return stream->pause();
    }

    bool CallInterface::resume() {
        std::lock_guard lock(mutex);
        return stream->resume();
    }

    bool CallInterface::mute() {
        std::lock_guard lock(mutex);
        return stream->mute();
    }

    bool CallInterface::unmute() {
        std::lock_guard lock(mutex);
        return stream->unmute();
    }

    void CallInterface::stop() {
        onStreamEnd(nullptr);
        onDisconnect(nullptr);
        std::lock_guard lock(mutex);
        stream->stop();
        if (connection) {
            connection->onConnectionChange(nullptr);
            connection->close();
        }
    }

    void CallInterface::changeStream(const MediaDescription& config) {
        std::lock_guard lock(mutex);
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

    uint64_t CallInterface::time() {
        std::lock_guard lock(mutex);
        return stream->time();
    }

    MediaState CallInterface::getState() {
        std::lock_guard lock(mutex);
        return stream->getState();
    }

    Stream::Status CallInterface::status() {
        std::lock_guard lock(mutex);
        return stream->status();
    }
} // ntgcalls