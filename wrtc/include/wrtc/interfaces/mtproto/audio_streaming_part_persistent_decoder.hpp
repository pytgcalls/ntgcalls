//
// Created by Laky64 on 14/04/25.
//

#pragma once
#include <wrtc/interfaces/mtproto/audio_streaming_part_persistent_decoder_state.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

namespace wrtc {

    class AudioStreamingPartPersistentDecoder {
        std::unique_ptr<AudioStreamingPartPersistentDecoderState> state;

        void maybeReset(const AVCodecParameters *codecParameters, AVRational timeBase);
    public:
        AudioStreamingPartPersistentDecoder() = default;

        ~AudioStreamingPartPersistentDecoder();

        int decode(AVCodecParameters const *codecParameters, AVRational timeBase, const AVPacket &packet, AVFrame *frame);
    };


} // wrtc
