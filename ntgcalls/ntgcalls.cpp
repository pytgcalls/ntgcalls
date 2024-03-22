//
// Created by Laky64 on 22/08/2023.
//

#include "ntgcalls.hpp"

#include "exceptions.hpp"
#include "instances/group_call.hpp"
#include "instances/p2p_call.hpp"
#include <pybind11/stl.h>

namespace ntgcalls {
    NTgCalls::NTgCalls() {
        updateQueue = std::make_unique<DispatchQueue>();
        hardwareInfo = std::make_unique<HardwareInfo>();
        INIT_ASYNC
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
                THREAD_SAFE
                (void) onEof(chatId, type);
                END_THREAD_SAFE
            });
        });
        if (connections[chatId]->type() & CallInterface::Type::Group) {
            SafeCall<GroupCall>(connections[chatId])->onUpgrade([this, chatId](const MediaState &state) {
                updateQueue->dispatch([this, chatId, state] {
                    THREAD_SAFE
                    (void) onChangeStatus(chatId, state);
                    END_THREAD_SAFE
                });
            });
        }
        connections[chatId]->onDisconnect([this, chatId]{
            updateQueue->dispatch([this, chatId] {
                THREAD_SAFE
                (void) onCloseConnection(chatId);
                END_THREAD_SAFE
                internalStop(chatId);
            });
        });
        if (connections[chatId]->type() & CallInterface::Type::P2P) {
            SafeCall<P2PCall>(connections[chatId])->onSignalingData([this, chatId](const bytes::binary& data) {
                updateQueue->dispatch([this, chatId, data] {
                    THREAD_SAFE
                    (void) onEmitData(chatId, CAST_BYTES(data));
                    END_THREAD_SAFE
                });
            });
        }
    }

    ASYNC_RETURN(bytes::vector) NTgCalls::createP2PCall(const int64_t userId, const int32_t &g, const BYTES(bytes::vector) &p, const BYTES(bytes::vector) &r, const std::optional<BYTES(bytes::vector)> &g_a_hash, const MediaDescription& media) {
        SMART_ASYNC(this, userId, g, p = CPP_BYTES(p, bytes::vector), r = CPP_BYTES(r, bytes::vector), g_a_hash = CPP_BYTES(g_a_hash, bytes::vector), media)
        std::lock_guard lock(mutex);
        CHECK_AND_THROW_IF_EXISTS(userId);
        connections[userId] = std::make_shared<P2PCall>();
        setupListeners(userId);
        const auto result = SafeCall<P2PCall>(connections[userId])->init(g, p, r, g_a_hash, media);
        END_ASYNC_RETURN_SAFE(CAST_BYTES(result))
    }

    ASYNC_RETURN(AuthParams) NTgCalls::exchangeKeys(const int64_t userId, const BYTES(bytes::vector) &p, const BYTES(bytes::vector) &g_a_or_b, const int64_t fingerprint) {
        SMART_ASYNC(this, userId, p = CPP_BYTES(p, bytes::vector), g_a_or_b = CPP_BYTES(g_a_or_b, bytes::vector), fingerprint)
        END_ASYNC_RETURN(SafeCall<P2PCall>(safeConnection(userId))->exchangeKeys(p, g_a_or_b, fingerprint))
    }

    ASYNC_RETURN(void) NTgCalls::connectP2P(const int64_t userId, const std::vector<wrtc::RTCServer>& servers, const std::vector<std::string>& versions) {
        SMART_ASYNC(this, userId, servers, versions)
        SafeCall<P2PCall>(safeConnection(userId))->connect(servers, versions);
        END_ASYNC
    }

    ASYNC_RETURN(std::string) NTgCalls::createCall(const int64_t chatId, const MediaDescription& media) {
        SMART_ASYNC(this, chatId, media)
        std::lock_guard lock(mutex);
        CHECK_AND_THROW_IF_EXISTS(chatId);
        connections[chatId] = std::make_shared<GroupCall>();
        setupListeners(chatId);
        END_ASYNC_RETURN(SafeCall<GroupCall>(connections[chatId])->init(media))
    }

    ASYNC_RETURN(void) NTgCalls::connect(const int64_t chatId, const std::string& params) {
        SMART_ASYNC(this, chatId, params)
        try {
            SafeCall<GroupCall>(safeConnection(chatId))->connect(params);
        } catch (TelegramServerError&) {
            internalStop(chatId);
            throw;
        }
        END_ASYNC
    }

    ASYNC_RETURN(void) NTgCalls::changeStream(const int64_t chatId, const MediaDescription& media) {
        SMART_ASYNC(this, chatId, media)
        safeConnection(chatId)->changeStream(media);
        END_ASYNC
    }

    ASYNC_RETURN(bool) NTgCalls::pause(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        END_ASYNC_RETURN(safeConnection(chatId)->pause())
    }

    ASYNC_RETURN(bool) NTgCalls::resume(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        END_ASYNC_RETURN(safeConnection(chatId)->resume())
    }

    ASYNC_RETURN(bool) NTgCalls::mute(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        END_ASYNC_RETURN(safeConnection(chatId)->mute())
    }

    ASYNC_RETURN(bool) NTgCalls::unmute(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        END_ASYNC_RETURN(safeConnection(chatId)->unmute())
    }

    ASYNC_RETURN(void) NTgCalls::stop(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        internalStop(chatId);
        END_ASYNC
    }

    void NTgCalls::onStreamEnd(const std::function<void(int64_t, Stream::Type)>& callback) {
        std::lock_guard lock(mutex);
        onEof = callback;
    }

    void NTgCalls::onUpgrade(const std::function<void(int64_t, MediaState)>& callback) {
        std::lock_guard lock(mutex);
        onChangeStatus = callback;
    }

    void NTgCalls::onDisconnect(const std::function<void(int64_t)>& callback) {
       std::lock_guard lock(mutex);
       onCloseConnection = callback;
    }

    void NTgCalls::onSignalingData(const std::function<void(int64_t, const BYTES(bytes::binary)&)>& callback) {
        std::lock_guard lock(mutex);
        onEmitData = callback;
    }

    ASYNC_RETURN(void) NTgCalls::sendSignalingData(const int64_t chatId, const BYTES(bytes::binary) &msgKey) {
        SMART_ASYNC(this, chatId, msgKey = CPP_BYTES(msgKey, bytes::binary))
        SafeCall<P2PCall>(safeConnection(chatId))->sendSignalingData(msgKey);
        END_ASYNC
    }

    ASYNC_RETURN(uint64_t) NTgCalls::time(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        END_ASYNC_RETURN(safeConnection(chatId)->time())
    }

    ASYNC_RETURN(MediaState) NTgCalls::getState(const int64_t chatId) {
        SMART_ASYNC(this, chatId)
        END_ASYNC_RETURN(safeConnection(chatId)->getState())
    }

    ASYNC_RETURN(double) NTgCalls::cpuUsage() const {
        SMART_ASYNC(this)
        END_ASYNC_RETURN(hardwareInfo->getCpuUsage())
    }

    ASYNC_RETURN(std::map<int64_t, Stream::Status>) NTgCalls::calls() {
        SMART_ASYNC(this)
        const std::unique_ptr<std::map<int64_t, Stream::Status>> statusList(new std::map<int64_t, Stream::Status>);
        std::lock_guard lock(mutex);
        for (const auto& [fst, snd] : connections) {
            statusList->emplace(fst, snd->status());
        }
        END_ASYNC_RETURN_SAFE(std::move(*statusList))
    }

    void NTgCalls::internalStop(const int64_t chatId) {
        safeConnection(chatId)->stop();
        std::lock_guard lock(mutex);
        if (connections.contains(chatId)) {
            connections.erase(connections.find(chatId));
        }
    }

    bool NTgCalls::exists(const int64_t chatId) const {
        return connections.contains(chatId);
    }

    std::shared_ptr<CallInterface> NTgCalls::safeConnection(const int64_t chatId) {
        std::lock_guard lock(mutex);
        if (!exists(chatId)) {
            throw ConnectionNotFound("Connection with chat id \"" + std::to_string(chatId) + "\" not found");
        }
        return connections[chatId];
    }

    Protocol NTgCalls::getProtocol() {
        return {
            92,
            92,
            true,
            true,
            Signaling::SupportedVersions(),
        };
    }

    template<typename DestCallType, typename BaseCallType>
    DestCallType* NTgCalls::SafeCall(const std::shared_ptr<BaseCallType>& call) {
        if (!call) {
            return nullptr;
        }
        if (typeid(*call) == typeid(DestCallType)) {
            return static_cast<DestCallType*>(call.get());
        }
        throw ConnectionError("Invalid call type");
    }

    std::string NTgCalls::ping() {
        return "pong";
    }
} // ntgcalls