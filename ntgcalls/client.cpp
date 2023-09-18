//
// Created by Laky64 on 12/08/2023.
//

#include "client.hpp"

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
        return offer;
    }

    std::string Client::init(MediaDescription config) {
        if (connection) {
            throw ConnectionError("Connection already made");
        }

        auto res = init();
        stream->setAVStream(config, true);
        audioSource = res.audioSource;
        for (auto &ssrc : res.sourceGroups) {
            sourceGroups.push_back(ssrc);
        }
        return res;
    }

    void Client::changeStream(MediaDescription config) {
        stream->setAVStream(config);
    }

    void Client::connect(const std::string& jsonData) {
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

        auto remoteDescription = wrtc::Description(
            wrtc::Description::Type::Answer,
            wrtc::SdpBuilder::fromConference(conference)
        );
        connection->setRemoteDescription(remoteDescription);

        wrtc::Sync<void> waitConnection;
        connection->onIceStateChange([&waitConnection](wrtc::IceState state) {
            switch (state) {
                case wrtc::IceState::Connected:
                    waitConnection.onSuccess();
                    break;
                case wrtc::IceState::Disconnected:
                case wrtc::IceState::Failed:
                case wrtc::IceState::Closed:
                    waitConnection.onFailed(ConnectionError("Connection failed to Telegram WebRTC"));
                    break;
                default:
                    break;
            }
        });
        waitConnection.wait();
        connection->onIceStateChange(nullptr);
        stream->start();
    }

    bool Client::pause() {
        return stream->pause();
    }

    bool Client::resume() {
        return stream->resume();
    }

    bool Client::mute() {
        return stream->mute();
    }

    bool Client::unmute() {
        return stream->unmute();
    }

    void Client::stop() {
        stream->stop();
        connection->close();
    }

    void Client::onStreamEnd(std::function<void(Stream::Type)> callback) {
        stream->onStreamEnd(callback);
    }

    void Client::onUpgrade(std::function<void(MediaState)> callback) {
        stream->onUpgrade(callback);
    }

    uint64_t Client::time() {
        return stream->time();
    }

    MediaState Client::getState() {
        return stream->getState();
    }

    Stream::Status Client::status() {
        return stream->status();
    }
}
