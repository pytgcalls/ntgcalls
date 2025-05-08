//
// Created by Laky64 on 14/04/25.
//

#pragma once
#include <memory>
#include <wrtc/interfaces/mtproto/wrapped_codec_parameters.hpp>

namespace wrtc {

    class AudioStreamingPartPersistentDecoderState {
        AVRational timeBase;
        int channelCount = 0;
        AVCodecContext *codecContext = nullptr;
        std::unique_ptr<WrappedCodecParameters> wrappedCodecParameters;

    public:
        AudioStreamingPartPersistentDecoderState(const AVCodecParameters *codecParameters, AVRational timeBase);

        ~AudioStreamingPartPersistentDecoderState();

        int decode(const AVPacket &packet, AVFrame *frame) const;

        AVRational getTimeBase() const;

        WrappedCodecParameters* getCodecParameters() const;
    };

} // wrtc
