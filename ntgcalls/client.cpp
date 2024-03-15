//
// Created by Laky64 on 12/08/2023.
//

#include "client.hpp"

#include "exceptions.hpp"
#include "utils/auth_key.hpp"
#include "utils/mod_exp_first.hpp"
#include "wrtc/utils/sync.hpp"

namespace ntgcalls {
    Client::Client() {
        stream = std::make_shared<Stream>();
    }

    Client::~Client() {
        stop();
        connection = nullptr;
        stream = nullptr;
        sourceGroups = {};
    }

    bytes::binary Client::init(const int32_t g, const bytes::binary& p, const bytes::binary& r, const bytes::binary& g_a_hash) {
        CHECK_CONNECTION_AND_THROW_ERROR();
        auto first = ModExpFirst(g, p, r);
        if (!first.modexp) {
            throw InvalidParams("Invalid modexp");
        }
        randomPower = std::move(first.randomPower);
        prime = p;
        if (g_a_hash) {
            this->g_a_hash = g_a_hash;
        }
        g_a_or_b = std::move(first.modexp);
        return g_a_hash ? g_a_or_b:g_a_or_b.Sha256();
    }

    AuthParams Client::confirmConnection(const bytes::binary& p, const bytes::binary& g_a_or_b, const int64_t& fingerprint, const std::vector<RTCServer>& servers, const std::vector<std::string> &versions) {
        if (connection) {
            throw ConnectionError("Connection already made");
        }
        if (!this->g_a_or_b) {
            throw ConnectionError("Connection not initialized");
        }
        if (g_a_hash) {
            if (!fingerprint) {
                throw InvalidParams("Fingerprint not found");
            }
            if (g_a_hash != g_a_or_b.Sha256()) {
                throw InvalidParams("Hash mismatch");
            }
        }
        const auto computedAuthKey = AuthKey::CreateAuthKey(
            g_a_or_b,
            randomPower,
            g_a_hash ? prime:p
        );
        if (!computedAuthKey) {
            throw ConnectionError("Could not create auth key");
        }
        auto authKey = AuthKey::FillData(computedAuthKey);
        const auto computedFingerprint = AuthKey::Fingerprint(authKey);
        if (g_a_hash && computedFingerprint != fingerprint) {
            throw InvalidParams("Fingerprint mismatch");
        }
        const auto res = init(servers);
        signaling = std::make_shared<SignalingConnection>(
            versions,
            connection->networkThread(),
            callType() == Type::Outgoing,
            authKey,
            [this](const bytes::binary& data) {
                (void) this->onEmitData(data);
            },
            [this](const bytes::binary& data) {
                this->processSignalingData(data);
            }
        );
        audioSource = res.audioSource;
        return {
            computedFingerprint,
            this->g_a_or_b,
        };
    }

    CallPayload Client::init(const std::vector<RTCServer>& servers) {
        connection = std::make_shared<wrtc::PeerConnection>(RTCServer::toIceServers(servers));
        stream->addTracks(connection);
        const auto offer = connection->createOffer(false, false);
        connection->setLocalDescription(offer);
        return CallPayload(offer);
    }

    std::string Client::init(const MediaDescription& config) {
        CHECK_CONNECTION_AND_THROW_ERROR();
        const auto res = init();
        stream->setAVStream(config, true);
        audioSource = res.audioSource;
        for (const auto &ssrc : res.sourceGroups) {
            sourceGroups.push_back(ssrc);
        }
        return res;
    }

    auto Client::changeStream(const MediaDescription& config) const -> void {
        stream->setAVStream(config);
    }

    void Client::connect(const std::string& jsonData) {
        if (!connection) {
            throw ConnectionError("Connection not initialized");
        }
        if (g_a_or_b) {
            throw ConnectionError("Connection already made");
        }
        auto data = json::parse(jsonData);
        if (!data["rtmp"].is_null()) {
            throw RTMPNeeded("Needed rtmp connection");
        }
        if (data["transport"].is_null()) {
            throw InvalidParams("Transport not found");
        }
        data = data["transport"];
        wrtc::Conference conference;
        try {
            conference = {
                {
                    data["ufrag"].get<std::string>(),
                    data["pwd"].get<std::string>()
                },
                audioSource,
                sourceGroups
            };
            for (const auto& item : data["fingerprints"].items()) {
                conference.transport.fingerprints.push_back({
                    item.value()["hash"],
                    item.value()["fingerprint"],
                });
            }
            for (const auto& item : data["candidates"].items()) {
                conference.transport.candidates.push_back({
                    item.value()["generation"].get<std::string>(),
                    item.value()["component"].get<std::string>(),
                    item.value()["protocol"].get<std::string>(),
                    item.value()["port"].get<std::string>(),
                    item.value()["ip"].get<std::string>(),
                    item.value()["foundation"].get<std::string>(),
                    item.value()["id"].get<std::string>(),
                    item.value()["priority"].get<std::string>(),
                    item.value()["type"].get<std::string>(),
                    item.value()["network"].get<std::string>()
                });
            }
        } catch (...) {
            throw InvalidParams("Invalid transport");
        }

        const auto remoteDescription = wrtc::Description(
            wrtc::Description::Type::Answer,
            wrtc::SdpBuilder::fromConference(conference)
        );
        connection->setRemoteDescription(remoteDescription);

        wrtc::Sync<void> waitConnection;
        connection->onIceStateChange([&](const wrtc::IceState state) {
            switch (state) {
            case wrtc::IceState::Connected:
                    if (!this->connected) waitConnection.onSuccess();
                    break;
                case wrtc::IceState::Disconnected:
                case wrtc::IceState::Failed:
                case wrtc::IceState::Closed:
                    if (!this->connected) {
                        waitConnection.onFailed(std::make_exception_ptr(TelegramServerError("Telegram Server is having some internal problems")));
                    } else {
                        connection->onIceStateChange(nullptr);
                        (void) this->onCloseConnection();
                    }
                    break;
                default:
                    break;
            }
        });
        waitConnection.wait();
        this->connected = true;
        stream->start();
    }

    bool Client::pause() const {
        return stream->pause();
    }

    bool Client::resume() const {
        return stream->resume();
    }

    bool Client::mute() const {
        return stream->mute();
    }

    bool Client::unmute() const {
        return stream->unmute();
    }

    void Client::stop() const {
        stream->stop();
        if (connection) {
            connection->onIceStateChange(nullptr);
            connection->close();
        }
    }

    void Client::onStreamEnd(const std::function<void(Stream::Type)> &callback) const {
        stream->onStreamEnd(callback);
    }

    void Client::onDisconnect(const std::function<void()> &callback) {
        this->onCloseConnection = callback;
    }

    void Client::onUpgrade(const std::function<void(MediaState)> &callback) const {
        stream->onUpgrade(callback);
    }

    void Client::onSignalingData(const std::function<void(bytes::binary)> &callback) {
        this->onEmitData = callback;
    }

    void Client::sendSignalingData(const bytes::binary& buffer) const {
        if (signaling) {
            signaling->receive(buffer);
        }
    }

    void Client::processSignalingData(const bytes::binary& buffer) const {
        if (signaling) {
            std::cout << "Signaling data: " << buffer << std::endl;
            json data = json::parse(std::string(static_cast<char*>(buffer), buffer.size()));
            if (data["@type"].is_null()) {
                return;
            }
        }
    }

    Client::Type Client::callType() const {
        if (g_a_or_b) {
            if (g_a_hash) {
                return Type::Incoming;
            }
            return Type::Outgoing;
        }
        if (connection) {
            return Type::Group;
        }
        return Type::Unknown;
    }

    uint64_t Client::time() const {
        return stream->time();
    }

    MediaState Client::getState() const {
        return stream->getState();
    }

    auto Client::status() const -> Stream::Status {
        return stream->status();
    }
}
