//
// Created by Laky64 on 14/04/25.
//

#include <rtc_base/logging.h>
#include <wrtc/interfaces/mtproto/audio_streaming_part_persistent_decoder.hpp>

namespace wrtc {
    void AudioStreamingPartPersistentDecoder::maybeReset(const AVCodecParameters* codecParameters, AVRational timeBase) {
        if (state) {
            bool isUpdated = false;
            if (!state->getCodecParameters()->isEqual(codecParameters)) {
                isUpdated = true;
            }
            if (state->getTimeBase().num != timeBase.num || state->getTimeBase().den != timeBase.den) {
                isUpdated = true;
            }
            if (!isUpdated) {
                return;
            }
        }

        state = std::make_unique<AudioStreamingPartPersistentDecoderState>(codecParameters, timeBase);
    }

    AudioStreamingPartPersistentDecoder::~AudioStreamingPartPersistentDecoder() {
        state = nullptr;
    }

    int AudioStreamingPartPersistentDecoder::decode(AVCodecParameters const* codecParameters, const AVRational timeBase, const AVPacket& packet, AVFrame* frame) {
        maybeReset(codecParameters, timeBase);
        if (!state) {
            return -1;
        }
        return state->decode(packet, frame);
    }
} // wrtc