//
// Created by Laky64 on 22/08/2023.
//

#include "ntgcalls.hpp"

#include "exceptions.hpp"

namespace ntgcalls {
    NTgCalls::NTgCalls() {
        updateQueue = std::make_shared<DispatchQueue>();
        hardwareInfo = std::make_shared<HardwareInfo>();
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
        hardwareInfo = nullptr;
        updateQueue = nullptr;
    }

    void NTgCalls::setupListeners(const int64_t chatId) {
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
        connections[chatId]->onSignalingData([this, chatId](const bytes::binary& data) {
            (void) onSignaling(chatId, data);
        });
    };

    bytes::binary NTgCalls::createP2PCall(const int64_t userId, const int32_t g, const bytes::binary& p, const bytes::binary& r, const bytes::binary& g_a_hash) {
        std::lock_guard lock(mutex);
        CHECK_AND_THROW_IF_EXISTS(userId);
        connections[userId] = std::make_shared<Client>();
        setupListeners(userId);
        return connections[userId]->init(g, p, r, g_a_hash);
    }

    AuthParams NTgCalls::confirmP2PCall(const int64_t userId, const bytes::binary& p, const bytes::binary& g_a_or_b, const int64_t& fingerprint) {
        std::lock_guard lock(mutex);
        return safeConnection(userId)->confirmConnection(p, g_a_or_b, fingerprint);
    }

    std::string NTgCalls::createCall(const int64_t chatId, const MediaDescription& media) {
        std::lock_guard lock(mutex);
        CHECK_AND_THROW_IF_EXISTS(chatId);
        connections[chatId] = std::make_shared<Client>();
        setupListeners(chatId);
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

    void NTgCalls::onSignalingData(const std::function<void(int64_t, bytes::binary)>& callback) {
        onSignaling = callback;
    }


    uint64_t NTgCalls::time(const int64_t chatId) {
        std::lock_guard lock(mutex);
        return safeConnection(chatId)->time();
    }

    MediaState NTgCalls::getState(const int64_t chatId) {
        std::lock_guard lock(mutex);
        return safeConnection(chatId)->getState();
    }

    double NTgCalls::cpuUsage() const {
        return hardwareInfo->getCpuUsage();
    }

    bool NTgCalls::exists(const int64_t chatId) const {
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

    Protocol NTgCalls::getProtocol() {
        return {
            92,
            92,
            true,
            true,
            {"6.0.0"},
        };
    }

    std::string NTgCalls::ping() {
        return "pong";
    }
} // ntgcalls