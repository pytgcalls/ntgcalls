//
// Created by Laky64 on 14/04/25.
//

#pragma once
#include <wrtc/interfaces/mtproto/audio_streaming_part_state.hpp>

namespace wrtc {

    class AudioStreamingPart {
        std::unique_ptr<AudioStreamingPartState> state;

    public:
        AudioStreamingPart(bytes::binary&& data, const std::string &container, bool isSingleChannel);

        ~AudioStreamingPart();

        int getRemainingMilliseconds() const;

        std::map<std::string, int32_t> getEndpointMapping() const;

        std::vector<AudioStreamingPartState::Channel> get10msPerChannel(AudioStreamingPartPersistentDecoder &persistentDecoder) const;
    };

} // wrtc
