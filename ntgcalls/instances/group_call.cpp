//
// Created by Laky64 on 15/03/2024.
//

#include "group_call.hpp"

#include "ntgcalls/exceptions.hpp"
#include "ntgcalls/models/call_payload.hpp"

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
        connection->setLocalDescription().wait();
        const auto payload = CallPayload(connection->localDescription().value());
        audioSource = payload.audioSource;
        for (const auto &ssrc : payload.sourceGroups) {
            sourceGroups.push_back(ssrc);
        }
        stream->setAVStream(config, true);
        return static_cast<std::string>(payload);
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
        connection->setRemoteDescription(
            wrtc::Description(
                wrtc::Description::SdpType::Offer,
                wrtc::SdpBuilder::fromConference(conference)
            )
        ).wait();
        std::promise<void> future;
        connection->onConnectionChange([&](const wrtc::PeerConnectionState state) {
            switch (state) {
            case wrtc::PeerConnectionState::Connected:
                if (!this->connected) {
                    connected = true;
                    stream->start();
                    future.set_value();
                }
                break;
            case wrtc::PeerConnectionState::Disconnected:
            case wrtc::PeerConnectionState::Failed:
            case wrtc::PeerConnectionState::Closed:
                connection->onConnectionChange(nullptr);
                if (!this->connected) {
                    future.set_exception(std::make_exception_ptr(TelegramServerError("Telegram Server is having some internal problems")));
                } else {
                    (void) onCloseConnection();
                }
                break;
            default:
                break;
            }
        });
        future.get_future().wait();
    }

    CallInterface::Type GroupCall::type() const {
        return Type::Group;
    }
} // ntgcalls