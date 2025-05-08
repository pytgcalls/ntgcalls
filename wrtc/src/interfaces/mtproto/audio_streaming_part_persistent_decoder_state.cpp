//
// Created by Laky64 on 14/04/25.
//

#include <rtc_base/logging.h>
#include <wrtc/interfaces/mtproto/audio_streaming_part_persistent_decoder_state.hpp>

namespace wrtc {
    AudioStreamingPartPersistentDecoderState::AudioStreamingPartPersistentDecoderState(const AVCodecParameters* codecParameters, const AVRational timeBase): timeBase(timeBase) {
        wrappedCodecParameters = std::make_unique<WrappedCodecParameters>(codecParameters);
        if (const AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id)) {
            codecContext = avcodec_alloc_context3(codec);
            if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
                avcodec_free_context(&codecContext);
                codecContext = nullptr;
            } else {
                codecContext->pkt_timebase = timeBase;
                channelCount = codecContext->ch_layout.nb_channels;
                if (avcodec_open2(codecContext, codec, nullptr) < 0) {
                    avcodec_free_context(&codecContext);
                    codecContext = nullptr;
                }
            }
        } else {
            RTC_LOG(LS_ERROR) << "Failed to find audio codec: " << codecParameters->codec_id;
        }
    }

    AudioStreamingPartPersistentDecoderState::~AudioStreamingPartPersistentDecoderState() {
        if (codecContext) {
            avcodec_free_context(&codecContext);
        }
        wrappedCodecParameters = nullptr;
    }

    int AudioStreamingPartPersistentDecoderState::decode(const AVPacket& packet, AVFrame* frame) const {
        if (!codecContext) {
            return -1;
        }

        int ret = avcodec_send_packet(codecContext, &packet);
        if (ret < 0) {
            return ret;
        }

        if (const int bytesPerSample = av_get_bytes_per_sample(codecContext->sample_fmt); bytesPerSample != 2 && bytesPerSample != 4) {
            return -1;
        }

        ret = avcodec_receive_frame(codecContext, frame);
        return ret;
    }

    AVRational AudioStreamingPartPersistentDecoderState::getTimeBase() const {
        return timeBase;
    }

    WrappedCodecParameters* AudioStreamingPartPersistentDecoderState::getCodecParameters() const {
        return wrappedCodecParameters.get();
    }
} // wrtc