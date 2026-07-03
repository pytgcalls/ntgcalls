//
// Created by Lauren on 14/04/25.
//

#pragma once
#include <memory>
#include <wrtc/interfaces/mtproto/wrapped_codec_parameters.hpp>

namespace wrtc::interfaces::mtproto {

    class AudioStreamingPartPersistentDecoderState {
        AVRational time_base_;
        int channel_count_ = 0;
        AVCodecContext* codec_context_ = nullptr;
        std::unique_ptr<WrappedCodecParameters> wrapped_codec_parameters_;

    public:
        AudioStreamingPartPersistentDecoderState(const AVCodecParameters* codec_parameters, AVRational time_base);

        ~AudioStreamingPartPersistentDecoderState();

        int decode(const AVPacket &packet, AVFrame* frame) const;

        [[nodiscard]] AVRational get_time_base() const;

        [[nodiscard]] WrappedCodecParameters* get_codec_parameters() const;
    };

} // wrtc::interfaces::mtproto
