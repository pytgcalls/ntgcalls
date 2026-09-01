//
// Created by Lauren on 14/04/25.
//

#include <rtc_base/logging.h>
#include <wrtc/interfaces/mtproto/audio_streaming_part_persistent_decoder.hpp>

namespace wrtc::interfaces::mtproto {
    void AudioStreamingPartPersistentDecoder::maybe_reset(const AVCodecParameters* codec_parameters, AVRational time_base) {
        if (state_) {
            bool is_updated = false;
            if (!state_->get_codec_parameters()->is_equal(codec_parameters)) {
                is_updated = true;
            }
            if (state_->get_time_base().num != time_base.num || state_->get_time_base().den != time_base.den) {
                is_updated = true;
            }
            if (!is_updated) {
                return;
            }
        }

        state_ = std::make_unique<AudioStreamingPartPersistentDecoderState>(codec_parameters, time_base);
    }

    AudioStreamingPartPersistentDecoder::~AudioStreamingPartPersistentDecoder() {
        state_ = nullptr;
    }

    int AudioStreamingPartPersistentDecoder::decode(AVCodecParameters const* codec_parameters, const AVRational time_base, const AVPacket& packet, AVFrame* frame) {
        maybe_reset(codec_parameters, time_base);
        if (!state_) {
            return -1;
        }
        return state_->decode(packet, frame);
    }
} // wrtc::interfaces::mtproto
