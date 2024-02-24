//
// Created by Laky64 on 22/08/2023.
//

#include "ntgcalls.hpp"

#include "exceptions.hpp"

namespace ntgcalls {
    NTgCalls::NTgCalls() {
        updateQueue = std::make_shared<DispatchQueue>();
    }

    NTgCalls::~NTgCalls() {
        std::lock_guard lock(mutex);
        // Temporary fix because macOs sucks and currently doesnt support Elements View
        // ReSharper disable once CppUseElementsView
        for (const auto& [fst, snd] : connections) {
            snd->onStreamEnd(nullptr);
            snd->stop();
        }
        connections = {};
        updateQueue = nullptr;
    }

    std::string NTgCalls::createCall(int64_t chatId, const MediaDescription& media) {
        std::lock_guard lock(mutex);
        if (exists(chatId)) {
            throw ConnectionError("Connection cannot be initialized more than once.");
        }
        connections[chatId] = std::make_shared<Client>();
        connections[chatId]->onStreamEnd([this, chatId](const Stream::Type &type) {
            updateQueue->dispatch([this, chatId, type] {
                (void) onEof(chatId, type);
            });
        });
        connections[chatId]->onUpgrade([this, chatId](const MediaState &state) {
            updateQueue->dispatch([this, chatId, state] {
                (void) onChangeStatus(chatId, state);
            });
        });
        connections[chatId]->onDisconnect([this, chatId]{
            updateQueue->dispatch([this, chatId] {
                (void) onCloseConnection(chatId);
                stop(chatId);
            });
        });
        return connections[chatId]->init(media);
    }

    void NTgCalls::connect(const int64_t chatId, const std::string& params) {
        std::lock_guard lock(mutex);
        try {
            safeConnection(chatId)->connect(params);
        } catch (TelegramServerError&) {
            stop(chatId);
            throw;
        }
    }

    void NTgCalls::changeStream(const int64_t chatId, const MediaDescription& media) {
        std::lock_guard lock(mutex);
        safeConnection(chatId)->changeStream(media);
    }

    bool NTgCalls::pause(const int64_t chatId) {
        std::lock_guard lock(mutex);
        return safeConnection(chatId)->pause();
    }

    bool NTgCalls::resume(const int64_t chatId) {
        std::lock_guard lock(mutex);
        return safeConnection(chatId)->resume();
    }

    bool NTgCalls::mute(const int64_t chatId) {
        std::lock_guard lock(mutex);
        return safeConnection(chatId)->mute();
    }

    bool NTgCalls::unmute(const int64_t chatId) {
        std::lock_guard lock(mutex);
        return safeConnection(chatId)->unmute();
    }

    void NTgCalls::stop(const int64_t chatId) {
        std::lock_guard lock(mutex);
        safeConnection(chatId)->stop();
        connections.erase(connections.find(chatId));
    }

    void NTgCalls::onStreamEnd(const std::function<void(int64_t, Stream::Type)>& callback) {
        onEof = callback;
    }

    void NTgCalls::onUpgrade(const std::function<void(int64_t, MediaState)>& callback) {
        onChangeStatus = callback;
    }

    void NTgCalls::onDisconnect(const std::function<void(int64_t)>& callback) {
        onCloseConnection = callback;
    }

    uint64_t NTgCalls::time(const int64_t chatId) {
        std::lock_guard lock(mutex);
        return safeConnection(chatId)->time();
    }

    MediaState NTgCalls::getState(const int64_t chatId) {
        std::lock_guard lock(mutex);
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

    std::map<int64_t, Stream::Status> NTgCalls::calls() {
        std::lock_guard lock(mutex);
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