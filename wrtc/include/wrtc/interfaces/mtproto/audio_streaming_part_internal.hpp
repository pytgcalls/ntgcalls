//
// Created by Lauren on 14/04/25.
//

#pragma once
#include <map>
#include <sstream>
#include <wrtc/interfaces/mtproto/audio_streaming_part_persistent_decoder.hpp>
#include <wrtc/interfaces/mtproto/avio_context_impl.hpp>

namespace wrtc::interfaces::mtproto {

    class AudioStreamingPartInternal {
        struct ChannelUpdate {
            int frame_index = 0;
            int id = 0;
            uint32_t ssrc = 0;
        };

        std::unique_ptr<AVIOContextImpl> av_io_context_;

        AVFormatContext* input_format_context_ = nullptr;
        AVPacket packet_ = {};
        AVFrame* frame_ = nullptr;
        AVCodecParameters* audio_codec_parameters_ = nullptr;

        bool did_read_to_end_ = false;

        int duration_in_milliseconds_ = 0;
        int stream_id_ = -1;
        int channel_count_ = 0;

        std::vector<ChannelUpdate> channel_updates_;
        std::map<std::string, int32_t> endpoint_mapping_;

        std::vector<int16_t> pcm_buffer_;
        int pcm_buffer_sample_offset_ = 0;
        int pcm_buffer_sample_size_ = 0;
        int read_sample_count_ = 0;

        template<typename Out>
        static void split_string(const std::string& s, const char delim, Out result) {
            std::istringstream iss(s);
            std::string item;
            while (std::getline(iss, item, delim)) {
                *result++ = item;
            }
        }

        static uint32_t string_to_uint32(const std::string& string);

        static std::optional<uint32_t> read_int32(const std::string& data, int& offset);

        static std::vector<ChannelUpdate> parse_channel_updates(const std::string& data, int& offset);

        static int16_t sample_float_to_int16(float sample);

        void fill_pcm_buffer(AudioStreamingPartPersistentDecoder& persistent_decoder);

    public:
        struct ReadPcmResult {
            int num_samples = 0;
            int num_channels = 0;
        };

        AudioStreamingPartInternal(bytes::binary&& data, const std::string& container);

        ~AudioStreamingPartInternal();

        [[nodiscard]] std::map<std::string, int32_t> get_endpoint_mapping() const;

        [[nodiscard]] std::vector<ChannelUpdate> get_channel_updates() const;

        [[nodiscard]] int get_duration_in_milliseconds() const;

        ReadPcmResult read_pcm(AudioStreamingPartPersistentDecoder& persistent_decoder, std::vector<int16_t>& out_pcm);
    };

} // wrtc::interfaces::mtproto
