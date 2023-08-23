//
// Created by Laky64 on 22/08/2023.
//

#include "ntgcalls.hpp"

namespace ntgcalls {

    std::string NTgCalls::createCall(int64_t chatId, MediaDescription media) {
        if (exists(chatId)) {
            throw ConnectionError("Connection cannot be initialized more than once.");
        }
        connections[chatId] = Client();
        return connections[chatId].init(media);
    }

    void NTgCalls::connect(int64_t chatId, std::string params) {
        safeConnection(chatId).connect(params);
    }

    void NTgCalls::changeStream(int64_t chatId, MediaDescription media) {
        safeConnection(chatId).changeStream(media);
    }

    void NTgCalls::pause(int64_t chatId) {
        safeConnection(chatId).pause();
    }

    void NTgCalls::resume(int64_t chatId) {
        safeConnection(chatId).resume();
    }

    void NTgCalls::mute(int64_t chatId) {
        safeConnection(chatId).mute();
    }

    void NTgCalls::unmute(int64_t chatId) {
        safeConnection(chatId).unmute();
    }

    void NTgCalls::stop(int64_t chatId) {
        safeConnection(chatId).stop();
    }

    bool NTgCalls::exists(int64_t chatId) {
        auto it = connections.find(chatId);
        return it != connections.end();
    }

    Client NTgCalls::safeConnection(int64_t chatId) {
        if (!exists(chatId)) {
            throw ConnectionError("Connection with chat id \"" + std::to_string(chatId) + "\" not found");
        }
        return connections[chatId];
    }

} // ntgcalls