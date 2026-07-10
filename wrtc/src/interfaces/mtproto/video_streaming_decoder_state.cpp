//
// Created by Lauren on 15/04/25.
//

#include <wrtc/interfaces/mtproto/video_streaming_decoder_state.hpp>

namespace wrtc::interfaces::mtproto {
    VideoStreamingDecoderState::VideoStreamingDecoderState(AVCodecContext* codec_context, const AVCodecParameters* codec_parameters, const AVRational pkt_timebase) :
    codec_context_(codec_context), timebase_(pkt_timebase) {
        parameters_ = avcodec_parameters_alloc();
        avcodec_parameters_copy(parameters_, codec_parameters);
    }

    VideoStreamingDecoderState::~VideoStreamingDecoderState() {
        if (codec_context_) {
            avcodec_free_context(&codec_context_);
        }
        if (parameters_) {
            avcodec_parameters_free(&parameters_);
        }
    }

    bool VideoStreamingDecoderState::are_codec_parameters_equal(const AVCodecParameters& lhs, AVCodecParameters const& rhs) {
        if (lhs.codec_id != rhs.codec_id) {
            return false;
        }
        if (lhs.extradata_size != rhs.extradata_size) {
            return false;
        }
        if (lhs.extradata_size != 0) {
            if (std::memcmp(lhs.extradata, rhs.extradata, lhs.extradata_size) != 0) {
                return false;
            }
        }
        if (lhs.format != rhs.format) {
            return false;
        }
        if (lhs.profile != rhs.profile) {
            return false;
        }
        if (lhs.level != rhs.level) {
            return false;
        }
        if (lhs.width != rhs.width) {
            return false;
        }
        if (lhs.height != rhs.height) {
            return false;
        }
        if (lhs.sample_aspect_ratio.num != rhs.sample_aspect_ratio.num) {
            return false;
        }
        if (lhs.sample_aspect_ratio.den != rhs.sample_aspect_ratio.den) {
            return false;
        }
        if (lhs.field_order != rhs.field_order) {
            return false;
        }
        if (lhs.color_range != rhs.color_range) {
            return false;
        }
        if (lhs.color_primaries != rhs.color_primaries) {
            return false;
        }
        if (lhs.color_trc != rhs.color_trc) {
            return false;
        }
        if (lhs.color_space != rhs.color_space) {
            return false;
        }
        if (lhs.chroma_location != rhs.chroma_location) {
            return false;
        }

        return true;
    }

    std::unique_ptr<VideoStreamingDecoderState> VideoStreamingDecoderState::create(const AVCodecParameters* codec_parameters, AVRational pkt_timebase) {
        const AVCodec* codec = avcodec_find_decoder(codec_parameters->codec_id);
        if (!codec) {
            return nullptr;
        }
        AVCodecContext* codec_context = avcodec_alloc_context3(codec);
        int ret = avcodec_parameters_to_context(codec_context, codec_parameters);
        if (ret < 0) {
            avcodec_free_context(&codec_context);
            return nullptr;
        }
        codec_context->pkt_timebase = pkt_timebase;
        ret = avcodec_open2(codec_context, codec, nullptr);
        if (ret < 0) {
            avcodec_free_context(&codec_context);
            return nullptr;
        }
        return std::make_unique<VideoStreamingDecoderState>(
            codec_context,
            codec_parameters,
            pkt_timebase
        );
    }

    bool VideoStreamingDecoderState::supports_decoding(const AVCodecParameters* codec_parameters, const AVRational pkt_timebase) const {
        if (!are_codec_parameters_equal(*parameters_, *codec_parameters)) {
            return false;
        }
        if (timebase_.num != pkt_timebase.num) {
            return false;
        }
        if (timebase_.den != pkt_timebase.den) {
            return false;
        }
        return true;
    }

    int VideoStreamingDecoderState::send_frame(const media::DecodableFrame* frame) const {
        if (frame) {
            return avcodec_send_packet(codec_context_, frame->get_packet()->get_packet());
        }
        return avcodec_send_packet(codec_context_, nullptr);
    }

    int VideoStreamingDecoderState::receive_frame(const media::VideoStreamingAVFrame* frame) const {
        return avcodec_receive_frame(codec_context_, frame->get_frame());
    }

    void VideoStreamingDecoderState::reset() const {
        avcodec_flush_buffers(codec_context_);
    }
} // wrtc::interfaces::mtproto