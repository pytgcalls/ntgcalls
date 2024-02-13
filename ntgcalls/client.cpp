//
// Created by Laky64 on 12/08/2023.
//

#include "client.hpp"

#include "exceptions.hpp"
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

    GroupCallPayload Client::init() {
        connection = std::make_shared<wrtc::PeerConnection>();

        stream->addTracks(connection);

        auto offer = connection->createOffer(true, true);
        connection->setLocalDescription(offer);
        return GroupCallPayload(offer);
    }

    std::string Client::init(const MediaDescription& config) {
        if (connection) {
            throw ConnectionError("Connection already made");
        }

        const auto res = init();
        stream->setAVStream(config, true);
        audioSource = res.audioSource;
        for (const auto &ssrc : res.sourceGroups) {
            sourceGroups.push_back(ssrc);
        }
        return std::string(res);
    }

    auto Client::changeStream(const MediaDescription& config) const -> void {
        stream->setAVStream(config);
    }

    void Client::connect(const std::string& jsonData) {
        if (!connection) {
            throw ConnectionError("Connection not initialized");
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
