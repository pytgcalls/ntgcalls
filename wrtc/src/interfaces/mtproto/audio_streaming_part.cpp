//
// Created by Lauren on 14/04/25.
//

#include <wrtc/interfaces/mtproto/audio_streaming_part.hpp>

namespace wrtc::interfaces::mtproto {
    AudioStreamingPart::AudioStreamingPart(bytes::binary&& data, const std::string& container, const bool is_single_channel) {
        if (!data.empty()) {
            state_ = std::make_unique<AudioStreamingPartState>(std::move(data), container, is_single_channel);
        }
    }

    AudioStreamingPart::~AudioStreamingPart() {
        state_ = nullptr;
    }

    int AudioStreamingPart::get_remaining_milliseconds() const {
        return state_ ? state_->get_remaining_milliseconds() : 0;
    }

    std::map<std::string, int32_t> AudioStreamingPart::get_endpoint_mapping() const {
        return state_ ? state_->get_endpoint_mapping() : std::map<std::string, int32_t>();
    }

    std::vector<AudioStreamingPartState::Channel> AudioStreamingPart::get_10ms_per_channel(AudioStreamingPartPersistentDecoder& persistent_decoder) const {
        return state_ ? state_->get_10ms_per_channel(persistent_decoder) : std::vector<AudioStreamingPartState::Channel>();
    }
} // wrtc::interfaces::mtproto