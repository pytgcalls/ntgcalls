//
// Created by Laky64 on 15/04/25.
//

#include <wrtc/interfaces/mtproto/video_streaming_decoder_state.hpp>

namespace wrtc {
    VideoStreamingDecoderState::VideoStreamingDecoderState(AVCodecContext* codecContext, const AVCodecParameters* codecParameters, const AVRational pktTimebase) :
    codecContext(codecContext), timebase(pktTimebase) {
        parameters = avcodec_parameters_alloc();
        avcodec_parameters_copy(parameters, codecParameters);
    }

    VideoStreamingDecoderState::~VideoStreamingDecoderState() {
        if (codecContext) {
            avcodec_free_context(&codecContext);
        }
        if (parameters) {
            avcodec_parameters_free(&parameters);
        }
    }

    bool VideoStreamingDecoderState::areCodecParametersEqual(const AVCodecParameters& lhs, AVCodecParameters const& rhs) {
        if (lhs.codec_id != rhs.codec_id) {
            return false;
        }
        if (lhs.extradata_size != rhs.extradata_size) {
            return false;
        }
        if (lhs.extradata_size != 0) {
            if (memcmp(lhs.extradata, rhs.extradata, lhs.extradata_size) != 0) {
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

    std::unique_ptr<VideoStreamingDecoderState> VideoStreamingDecoderState::create(const AVCodecParameters* codecParameters, AVRational pktTimebase) {
        const AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
        if (!codec) {
            return nullptr;
        }
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        int ret = avcodec_parameters_to_context(codecContext, codecParameters);
        if (ret < 0) {
            avcodec_free_context(&codecContext);
            return nullptr;
        }
        codecContext->pkt_timebase = pktTimebase;
        ret = avcodec_open2(codecContext, codec, nullptr);
        if (ret < 0) {
            avcodec_free_context(&codecContext);
            return nullptr;
        }
        return std::make_unique<VideoStreamingDecoderState>(
            codecContext,
            codecParameters,
            pktTimebase
        );
    }

    bool VideoStreamingDecoderState::supportsDecoding(const AVCodecParameters* codecParameters, const AVRational pktTimebase) const {
        if (!areCodecParametersEqual(*parameters, *codecParameters)) {
            return false;
        }
        if (timebase.num != pktTimebase.num) {
            return false;
        }
        if (timebase.den != pktTimebase.den) {
            return false;
        }
        return true;
    }

    int VideoStreamingDecoderState::sendFrame(const DecodableFrame* frame) const {
        if (frame) {
            return avcodec_send_packet(codecContext, frame->getPacket()->getPacket());
        }
        return avcodec_send_packet(codecContext, nullptr);
    }

    int VideoStreamingDecoderState::receiveFrame(const VideoStreamingAVFrame* frame) const {
        return avcodec_receive_frame(codecContext, frame->getFrame());
    }

    void VideoStreamingDecoderState::reset() const {
        avcodec_flush_buffers(codecContext);
    }
} // wrtc