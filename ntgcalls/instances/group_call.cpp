//
// Created by Laky64 on 15/03/2024.
//

#include "group_call.hpp"

#include <future>

#include "ntgcalls/exceptions.hpp"
#include "ntgcalls/models/call_payload.hpp"

namespace ntgcalls {
    GroupCall::~GroupCall() {
        sourceGroups.clear();
    }

    std::string GroupCall::init(const MediaDescription& config) {
        RTC_LOG(LS_INFO) << "Initializing group call";
        std::lock_guard lock(mutex);
        if (connection) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        connection = std::make_unique<wrtc::PeerConnection>();
        stream->addTracks(connection);
        try {
            Safe<wrtc::PeerConnection>(connection)->setLocalDescription();
        } catch (wrtc::RTCException&) {
            RTC_LOG(LS_ERROR) << "Failed to set local description";
            throw ConnectionError("Failed to set local description");
        }
        RTC_LOG(LS_INFO) << "Group call initialized";
        const auto payload = CallPayload(Safe<wrtc::PeerConnection>(connection)->localDescription().value());
        audioSource = payload.audioSource;
        for (const auto &ssrc : payload.sourceGroups) {
            sourceGroups.push_back(ssrc);
        }
        stream->setAVStream(config, true);
        RTC_LOG(LS_INFO) << "AVStream settings applied";
        return static_cast<std::string>(payload);
    }

    void GroupCall::connect(const std::string& jsonData) {
        RTC_LOG(LS_INFO) << "Connecting to group call";
        std::lock_guard lock(mutex);
        if (!connection) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionError("Connection not initialized");
        }
        json data;
        try {
            data = json::parse(jsonData);
        } catch (std::exception& e) {
            RTC_LOG(LS_ERROR) << "Invalid JSON: " << e.what();
            throw InvalidParams("Invalid JSON");
        }
        if (!data["rtmp"].is_null()) {
            RTC_LOG(LS_ERROR) << "RTMP connection needed";
            throw RTMPNeeded("Needed rtmp connection");
        }
        if (data["transport"].is_null()) {
            RTC_LOG(LS_ERROR) << "Transport not found";
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
        RTC_LOG(LS_INFO) << "Setting remote description";
        try {
            Safe<wrtc::PeerConnection>(connection)->setRemoteDescription(
                wrtc::Description(
                    wrtc::Description::SdpType::Answer,
                    wrtc::SdpBuilder::fromConference(conference)
                )
            );
        } catch (wrtc::RTCException&) {
            throw TelegramServerError("Telegram Server is having some internal problems");
        }
        RTC_LOG(LS_INFO) << "Remote description set";
        std::promise<void> promise;
        connection->onConnectionChange([&](const wrtc::ConnectionState state) {
            switch (state) {
            case wrtc::ConnectionState::Connected:
                if (!connected) {
                    RTC_LOG(LS_INFO) << "Connection established";
                    connected = true;
                    stream->start();
                    RTC_LOG(LS_INFO) << "Stream started";
                    promise.set_value();
                }
                break;
            case wrtc::ConnectionState::Disconnected:
            case wrtc::ConnectionState::Failed:
            case wrtc::ConnectionState::Closed:
                workerThread->PostTask([this] {
                    connection->onConnectionChange(nullptr);
                });
                if (!connected) {
                    RTC_LOG(LS_ERROR) << "Connection failed";
                    promise.set_exception(std::make_exception_ptr(TelegramServerError("Telegram Server is having some internal problems")));
                } else {
                    RTC_LOG(LS_INFO) << "Connection closed";
                    (void) onCloseConnection();
                }
                break;
            default:
                break;
            }
        });
        if (promise.get_future().wait_for(std::chrono::seconds(60)) != std::future_status::ready) {
            RTC_LOG(LS_ERROR) << "Connection timeout";
            throw TelegramServerError("Connection timeout");
        }
    }

    void GroupCall::onUpgrade(const std::function<void(MediaState)>& callback) {
        std::lock_guard lock(mutex);
        stream->onUpgrade(callback);
    }

    CallInterface::Type GroupCall::type() const {
        return Type::Group;
    }
} // ntgcalls