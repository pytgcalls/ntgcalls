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

    void CallInterface::stop() const {
        stream->stop();
        if (connection) {
            connection->onIceStateChange(nullptr);
            connection->close();
        }
    }

    void CallInterface::changeStream(const MediaDescription& config) const {
        stream->setAVStream(config);
    }

    void CallInterface::onStreamEnd(const std::function<void(Stream::Type)>& callback) const {
        stream->onStreamEnd(callback);
    }

    void CallInterface::onDisconnect(const std::function<void()>& callback) {
        onCloseConnection = callback;
    }

    void CallInterface::onUpgrade(const std::function<void(MediaState)>& callback) const {
        stream->onUpgrade(callback);
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