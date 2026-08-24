//
// Created by Lauren on 22/03/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/signaling/messages/media_state_message.hpp>

namespace ntgcalls::signaling::messages {
    bytes::binary MediaStateMessage::serialize() const {
        const json payload{
            {"@type", "MediaState"},
            {"muted", is_muted},
            {"lowBattery", is_battery_low},
            {"videoState", parse_video_state(video_state)},
            {"videoRotation", video_rotation},
            {"screencastState", parse_video_state(screencast_state)},
        };
        return bytes::make_binary(payload.dump());
    }

    std::unique_ptr<MediaStateMessage> MediaStateMessage::deserialize(const bytes::binary& data) {
        json j = json::parse(data.begin(), data.end());
        auto message = std::make_unique<MediaStateMessage>();
        message->is_muted = j["muted"];
        message->is_battery_low = j["lowBattery"];
        message->video_state = parse_video_state(j["videoState"].get<std::string>());
        message->screencast_state = parse_video_state(j["screencastState"].get<std::string>());
        message->video_rotation = j["videoRotation"];
        return std::move(message);
    }

    std::string MediaStateMessage::parse_video_state(const VideoState state) {
        switch (state) {
        case VideoState::Inactive:
            return "inactive";
        case VideoState::Suspended:
            return "suspended";
        case VideoState::Active:
            return "active";
        }
        throw InvalidParams("Invalid video state");
    }

    MediaStateMessage::VideoState MediaStateMessage::parse_video_state(const std::string& state) {
        if (state == "inactive") {
            return VideoState::Inactive;
        }
        if (state == "suspended") {
            return VideoState::Suspended;
        }
        if (state == "active") {
            return VideoState::Active;
        }
        throw InvalidParams("Invalid video state");
    }
} // ntgcalls::signaling::messages
