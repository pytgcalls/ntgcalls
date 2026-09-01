//
// Created by Lauren on 14/04/25.
//

#include <rtc_base/logging.h>
#include <wrtc/interfaces/mtproto/audio_streaming_part_persistent_decoder_state.hpp>

namespace wrtc::interfaces::mtproto {
    AudioStreamingPartPersistentDecoderState::AudioStreamingPartPersistentDecoderState(const AVCodecParameters* codec_parameters, const AVRational time_base): time_base_(time_base) {
        wrapped_codec_parameters_ = std::make_unique<WrappedCodecParameters>(codec_parameters);
        if (const AVCodec* codec = avcodec_find_decoder(codec_parameters->codec_id)) {
            codec_context_ = avcodec_alloc_context3(codec);
            if (avcodec_parameters_to_context(codec_context_, codec_parameters) < 0) {
                avcodec_free_context(&codec_context_);
                codec_context_ = nullptr;
            } else {
                codec_context_->pkt_timebase = time_base;
                channel_count_ = codec_context_->ch_layout.nb_channels;
                if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
                    avcodec_free_context(&codec_context_);
                    codec_context_ = nullptr;
                }
            }
        } else {
            RTC_LOG(LS_ERROR) << "Failed to find audio codec: " << codec_parameters->codec_id;
        }
    }

    AudioStreamingPartPersistentDecoderState::~AudioStreamingPartPersistentDecoderState() {
        if (codec_context_) {
            avcodec_free_context(&codec_context_);
        }
        wrapped_codec_parameters_ = nullptr;
    }

    int AudioStreamingPartPersistentDecoderState::decode(const AVPacket& packet, AVFrame* frame) const {
        if (!codec_context_) {
            return -1;
        }

        int ret = avcodec_send_packet(codec_context_, &packet);
        if (ret < 0) {
            return ret;
        }

        if (const int bytes_per_sample = av_get_bytes_per_sample(codec_context_->sample_fmt); bytes_per_sample != 2 && bytes_per_sample != 4) {
            return -1;
        }

        ret = avcodec_receive_frame(codec_context_, frame);
        return ret;
    }

    AVRational AudioStreamingPartPersistentDecoderState::get_time_base() const {
        return time_base_;
    }

    WrappedCodecParameters* AudioStreamingPartPersistentDecoderState::get_codec_parameters() const {
        return wrapped_codec_parameters_.get();
    }
} // wrtc::interfaces::mtproto
