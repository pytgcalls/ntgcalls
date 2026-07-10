//
// Created by Lauren on 14/04/25.
//

#pragma once
#include <wrtc/interfaces/mtproto/audio_streaming_part_state.hpp>

namespace wrtc::interfaces::mtproto {

    class AudioStreamingPart {
        std::unique_ptr<AudioStreamingPartState> state_;

    public:
        AudioStreamingPart(bytes::binary&& data, const std::string &container, bool is_single_channel);

        ~AudioStreamingPart();

        [[nodiscard]] int get_remaining_milliseconds() const;

        [[nodiscard]] std::map<std::string, int32_t> get_endpoint_mapping() const;

        std::vector<AudioStreamingPartState::Channel> get_10ms_per_channel(AudioStreamingPartPersistentDecoder &persistent_decoder) const;
    };

} // wrtc::interfaces::mtproto
