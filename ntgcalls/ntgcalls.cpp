//
// Created by Laky64 on 22/08/2023.
//

#include "ntgcalls.hpp"

namespace ntgcalls {
    std::string NTgCalls::createCall(int64_t chatId, MediaDescription media) {
        if (exists(chatId)) {
            throw ConnectionError("Connection cannot be initialized more than once.");
        }
        connections[chatId] = std::make_shared<Client>();
        return connections[chatId]->init(media);
    }

    NTgCalls::~NTgCalls() {
        for (auto conn : connections) {
            conn.second->stop();
        }
        connections = {};
    }

    void NTgCalls::connect(int64_t chatId, std::string params) {
        safeConnection(chatId)->connect(params);
    }

    void NTgCalls::changeStream(int64_t chatId, MediaDescription media) {
        safeConnection(chatId)->changeStream(media);
    }

    bool NTgCalls::pause(int64_t chatId) {
        return safeConnection(chatId)->pause();
    }

    bool NTgCalls::resume(int64_t chatId) {
        return safeConnection(chatId)->resume();
    }

    bool NTgCalls::mute(int64_t chatId) {
        return safeConnection(chatId)->mute();
    }

    bool NTgCalls::unmute(int64_t chatId) {
        return safeConnection(chatId)->unmute();
    }

    void NTgCalls::stop(int64_t chatId) {
        safeConnection(chatId)->stop();
        connections.erase(connections.find(chatId));
    }

    void NTgCalls::onStreamEnd(int64_t chatId, std::function<void(int64_t, Stream::Type)> &callback) {
        safeConnection(chatId)->onStreamEnd([chatId, &callback](Stream::Type type) {
            callback(chatId, type);
        });
    }

    void NTgCalls::onUpgrade(int64_t chatId, std::function<void(int64_t, MediaState)> &callback) {
        safeConnection(chatId)->onUpgrade([chatId, &callback](MediaState state) {
            callback(chatId, state);
        });
    }

    uint64_t NTgCalls::time(int64_t chatId) {
        return safeConnection(chatId)->time();
    }

    bool NTgCalls::exists(int64_t chatId) {
        return connections.find(chatId) != connections.end();
    }

    std::shared_ptr<Client> NTgCalls::safeConnection(int64_t chatId) {
        if (!exists(chatId)) {
            throw ConnectionError("Connection with chat id \"" + std::to_string(chatId) + "\" not found");
        }
        return connections[chatId];
    }

    std::string NTgCalls::ping() {
        return "pong";
    }
} // ntgcalls