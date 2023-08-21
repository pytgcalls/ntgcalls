//
// Created by Laky64 on 12/08/2023.
//

#include <iostream>
#include "client.hpp"

namespace ntgcalls {
    GroupCallPayload Client::init() {
        connection = std::make_shared<wrtc::PeerConnection>();

        stream->addTracks(connection);

        auto offer = connection->createOffer(true, true);
        connection->setLocalDescription(offer);
        return offer;
    }

    std::string Client::createCall(std::string audioPath) {
        if (connection) {
            throw ConnectionError("Connection already made");
        }

        stream = std::make_shared<Stream>();
        auto test = std::make_shared<FileReader>(audioPath);
        stream->setAVStream(StreamConfig{
            AudioConfig(test, 48000, 16, 2),
        });
        auto res = init();
        audioSource = res.audioSource;
        for (auto &ssrc : res.sourceGroups) {
            sourceGroups.push_back(ssrc);
        }
        return res;
    }

    void Client::setRemoteCallParams(const std::string& jsonData) {
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
        stream->start();
    }
}
