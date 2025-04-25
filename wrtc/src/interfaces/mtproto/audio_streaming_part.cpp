//
// Created by Laky64 on 14/04/25.
//

#include <wrtc/interfaces/mtproto/audio_streaming_part.hpp>

namespace wrtc {
    AudioStreamingPart::AudioStreamingPart(bytes::binary&& data, const std::string& container, const bool isSingleChannel) {
        if (!data.empty()) {
            state = std::make_unique<AudioStreamingPartState>(std::move(data), container, isSingleChannel);
        }
    }

    AudioStreamingPart::~AudioStreamingPart() {
        state = nullptr;
    }

    int AudioStreamingPart::getRemainingMilliseconds() const {
        return state ? state->getRemainingMilliseconds() : 0;
    }

    std::map<std::string, int32_t> AudioStreamingPart::getEndpointMapping() const {
        return state ? state->getEndpointMapping() : std::map<std::string, int32_t>();
    }

    std::vector<AudioStreamingPartState::Channel> AudioStreamingPart::get10msPerChannel(AudioStreamingPartPersistentDecoder& persistentDecoder) const {
        return state ? state->get10msPerChannel(persistentDecoder) : std::vector<AudioStreamingPartState::Channel>();
    }
} // wrtc