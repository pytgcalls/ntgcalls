//
// Created by Laky64 on 22/03/2024.
//

#include "media_state_message.hpp"

#include <nlohmann/json.hpp>

#include "ntgcalls/exceptions.hpp"

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
} // signaling
