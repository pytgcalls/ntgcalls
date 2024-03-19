//
// Created by Laky64 on 15/03/2024.
//

#include "group_call.hpp"

#include "ntgcalls/exceptions.hpp"
#include "ntgcalls/models/call_payload.hpp"
#include "wrtc/utils/sync.hpp"

namespace ntgcalls {
    GroupCall::~GroupCall() {
        sourceGroups.clear();
    }

    std::string GroupCall::init(const MediaDescription& config) {
        if (connection) {
            throw ConnectionError("Connection already made");
        }
        connection = std::make_unique<wrtc::PeerConnection>();
        stream->addTracks(connection);
        const std::optional offer = connection->createOffer(false, false);
        connection->setLocalDescription(offer);
        const auto payload = CallPayload(offer.value());
        stream->setAVStream(config, true);
        audioSource = payload.audioSource;
        for (const auto &ssrc : payload.sourceGroups) {
            sourceGroups.push_back(ssrc);
        }
        return payload;
    }

    void GroupCall::connect(const std::string& jsonData) {
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
                    data["ufrag"],
                    data["pwd"]
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
                    item.value()["generation"],
                    item.value()["component"],
                    item.value()["protocol"],
                    item.value()["port"],
                    item.value()["ip"],
                    item.value()["foundation"],
                    item.value()["id"],
                    item.value()["priority"],
                    item.value()["type"],
                    item.value()["network"]
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
        connection->onConnectionChange([&](const wrtc::PeerConnectionState state) {
            switch (state) {
            case wrtc::PeerConnectionState::Connected:
                if (!this->connected) waitConnection.onSuccess();
                break;
            case wrtc::PeerConnectionState::Disconnected:
            case wrtc::PeerConnectionState::Failed:
            case wrtc::PeerConnectionState::Closed:
                if (!this->connected) {
                    waitConnection.onFailed(std::make_exception_ptr(TelegramServerError("Telegram Server is having some internal problems")));
                } else {
                    connection->onConnectionChange(nullptr);
                    (void) this->onCloseConnection();
                }
                break;
            default:
                break;
            }
        });
        waitConnection.wait();
        connected = true;
        stream->start();
    }

    CallInterface::Type GroupCall::type() const {
        return Type::Group;
    }
} // ntgcalls