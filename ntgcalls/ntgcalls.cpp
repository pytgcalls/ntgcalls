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
        connections[chatId]->onStreamEnd([this, chatId](Stream::Type type) {
            onEof(chatId, type);
        });
        connections[chatId]->onUpgrade([this, chatId](MediaState state) {
            onChangeStatus(chatId, state);
        });
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

    void NTgCalls::onStreamEnd(std::function<void(int64_t, Stream::Type)> callback) {
        onEof = callback;
    }

    void NTgCalls::onUpgrade(std::function<void(int64_t, MediaState)> callback) {
        onChangeStatus = callback;
    }

    uint64_t NTgCalls::time(int64_t chatId) {
        return safeConnection(chatId)->time();
    }

    MediaState NTgCalls::getState(int64_t chatId) {
        return safeConnection(chatId)->getState();
    }

    bool NTgCalls::exists(int64_t chatId) {
        return connections.find(chatId) != connections.end();
    }

    std::shared_ptr<Client> NTgCalls::safeConnection(int64_t chatId) {
        if (!exists(chatId)) {
            throw ConnectionNotFound("Connection with chat id \"" + std::to_string(chatId) + "\" not found");
        }
        return connections[chatId];
    }

    std::map<int64_t, Stream::Status> NTgCalls::calls() {
        std::map<int64_t, Stream::Status> statusList;
        for (auto conn : connections) {
            statusList[conn.first] = conn.second->status();
        }
        return statusList;
    }

    std::string NTgCalls::ping() {
        return "pong";
    }
} // ntgcalls