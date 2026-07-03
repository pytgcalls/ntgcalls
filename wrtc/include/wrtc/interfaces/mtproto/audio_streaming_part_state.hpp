//
// Created by Lauren on 14/04/25.
//

#pragma once
#include <set>
#include <wrtc/interfaces/mtproto/audio_streaming_part_internal.hpp>

namespace wrtc::interfaces::mtproto {

    class AudioStreamingPartState {
        struct ChannelMapping {
            uint32_t ssrc = 0;
            int channel_index = 0;

            ChannelMapping(const uint32_t ssrc, const int channel_index) : ssrc(ssrc), channel_index(channel_index) {}
        };

        std::set<uint32_t> all_ssrcs_;
        bool did_read_to_end_ = false;
        bool is_single_channel_ = false;
        int remaining_milliseconds_ = 0;
        std::vector<int16_t> pcm10ms_;
        std::vector<ChannelMapping> current_channel_mapping_;
        int frame_index_ = 0;
        std::unique_ptr<AudioStreamingPartInternal> parsed_part_;

        void update_current_mapping(uint32_t ssrc, int channel_index);

        [[nodiscard]] std::optional<int> get_current_mapped_channel_index(uint32_t ssrc) const;

    public:
        struct Channel {
            uint32_t ssrc = 0;
            std::vector<int16_t> pcm_data;
        };

        AudioStreamingPartState(bytes::binary&& data, const std::string &container, bool is_single_channel);

        ~AudioStreamingPartState();

        std::vector<Channel> get_10ms_per_channel(AudioStreamingPartPersistentDecoder &persistent_decoder);

        [[nodiscard]] int get_remaining_milliseconds() const;

        [[nodiscard]] std::map<std::string, int32_t> get_endpoint_mapping() const;
    };

} // wrtc::interfaces::mtproto
