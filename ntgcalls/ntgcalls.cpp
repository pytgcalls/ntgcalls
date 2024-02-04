//
// Created by Laky64 on 22/08/2023.
//

#include "ntgcalls.hpp"

#include "exceptions.hpp"

namespace ntgcalls {
    std::string NTgCalls::createCall(int64_t chatId, const MediaDescription& media) {
        if (exists(chatId)) {
            throw ConnectionError("Connection cannot be initialized more than once.");
        }
        connections[chatId] = std::make_shared<Client>();
        connections[chatId]->onStreamEnd([this, chatId](const Stream::Type type) {
            (void) onEof(chatId, type);
        });
        connections[chatId]->onUpgrade([this, chatId](const MediaState state) {
            (void) onChangeStatus(chatId, state);
        });
        return connections[chatId]->init(media);
    }

    NTgCalls::~NTgCalls() {
        // Temporary fix because macOs sucks and currently doesnt support Elements View
        // ReSharper disable once CppUseElementsView
        for (const auto& [fst, snd] : connections) {
            snd->stop();
        }
        connections = {};
    }

    void NTgCalls::connect(const int64_t chatId, const std::string& params) {
        safeConnection(chatId)->connect(params);
    }

    void NTgCalls::changeStream(const int64_t chatId, const MediaDescription& media) {
        safeConnection(chatId)->changeStream(media);
    }

    bool NTgCalls::pause(const int64_t chatId) {
        return safeConnection(chatId)->pause();
    }

    bool NTgCalls::resume(const int64_t chatId) {
        return safeConnection(chatId)->resume();
    }

    bool NTgCalls::mute(const int64_t chatId) {
        return safeConnection(chatId)->mute();
    }

    bool NTgCalls::unmute(const int64_t chatId) {
        return safeConnection(chatId)->unmute();
    }

    void NTgCalls::stop(const int64_t chatId) {
        safeConnection(chatId)->stop();
        connections.erase(connections.find(chatId));
    }

    void NTgCalls::onStreamEnd(const std::function<void(int64_t, Stream::Type)>& callback) {
        onEof = callback;
    }

    void NTgCalls::onUpgrade(const std::function<void(int64_t, MediaState)>& callback) {
        onChangeStatus = callback;
    }

    uint64_t NTgCalls::time(const int64_t chatId) {
        return safeConnection(chatId)->time();
    }

    MediaState NTgCalls::getState(const int64_t chatId) {
        return safeConnection(chatId)->getState();
    }

    bool NTgCalls::exists(const int64_t chatId) const
    {
        return connections.contains(chatId);
    }

    std::shared_ptr<Client> NTgCalls::safeConnection(const int64_t chatId) {
        if (!exists(chatId)) {
            throw ConnectionNotFound("Connection with chat id \"" + std::to_string(chatId) + "\" not found");
        }
        return connections[chatId];
    }

    std::map<int64_t, Stream::Status> NTgCalls::calls() const {
        std::map<int64_t, Stream::Status> statusList;
        for (const auto& [fst, snd] : connections) {
            statusList[fst] = snd->status();
        }
        return statusList;
    }

    std::string NTgCalls::ping() {
        return "pong";
    }
} // ntgcalls