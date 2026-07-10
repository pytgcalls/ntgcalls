//
// Created by Lauren on 14/04/25.
//

#pragma once
#include <wrtc/interfaces/mtproto/audio_streaming_part_persistent_decoder_state.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

namespace wrtc::interfaces::mtproto {

    class AudioStreamingPartPersistentDecoder {
        std::unique_ptr<AudioStreamingPartPersistentDecoderState> state_;

        void maybe_reset(const AVCodecParameters* codec_parameters, AVRational time_base);

    public:
        AudioStreamingPartPersistentDecoder() = default;

        ~AudioStreamingPartPersistentDecoder();

        int decode(AVCodecParameters const* codec_parameters, AVRational time_base, const AVPacket &packet, AVFrame* frame);
    };


} // wrtc::interfaces::mtproto
