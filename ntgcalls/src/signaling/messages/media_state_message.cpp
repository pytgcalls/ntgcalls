//
// Created by Laky64 on 22/03/2024.
//

#include <ntgcalls/signaling/messages/media_state_message.hpp>

#include <nlohmann/json.hpp>

#include <ntgcalls/exceptions.hpp>

namespace signaling {
    bytes::binary MediaStateMessage::serialize() const {
        return bytes::make_binary(to_string(json{
            {"@type", "MediaState"},
            {"muted", isMuted},
            {"lowBattery", isBatteryLow},
            {"videoState", parseVideoState(videoState)},
            {"videoRotation", videoRotation},
            {"screencastState", parseVideoState(screencastState)}
        }));
    }

    std::unique_ptr<MediaStateMessage> MediaStateMessage::deserialize(const bytes::binary& data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<MediaStateMessage>();
        message->isMuted = j["muted"];
        message->isBatteryLow = j["lowBattery"];
        message->videoState = parseVideoState(j["videoState"].get<std::string>());
        message->screencastState = parseVideoState(j["screencastState"].get<std::string>());
        message->videoRotation = j["videoRotation"];
        return std::move(message);
    }

    std::string MediaStateMessage::parseVideoState(const VideoState state) {
        switch (state) {
        case VideoState::Inactive:
            return "inactive";
        case VideoState::Suspended:
            return "suspended";
        case VideoState::Active:
            return "active";
        }
        throw ntgcalls::InvalidParams("Invalid video state");
    }

    MediaStateMessage::VideoState MediaStateMessage::parseVideoState(const std::string& state) {
        if (state == "inactive") {
            return VideoState::Inactive;
        }
        if (state == "suspended") {
            return VideoState::Suspended;
        }
        if (state == "active") {
            return VideoState::Active;
        }
        throw ntgcalls::InvalidParams("Invalid video state");
    }
} // signaling
