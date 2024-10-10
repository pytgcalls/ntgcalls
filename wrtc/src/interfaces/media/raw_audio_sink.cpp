//
// Created by Laky64 on 07/10/24.
//

#include <utility>
#include <wrtc/interfaces/media/raw_audio_sink.hpp>

namespace wrtc {
    void RawAudioSink::OnData(const Data& audio) {
        if (callbackData) {
            auto frame = std::make_unique<AudioFrame>(ssrc);
            frame->size = audio.samples_per_channel * audio.channels * sizeof(int16_t);
            frame->data = audio.data;
            frame->sampleRate = audio.sample_rate;
            frame->channels = audio.channels;
            callbackData(std::move(frame));
        }
    }

    void RawAudioSink::setRemoteAudioSink(const uint32_t ssrc, std::function<void(std::unique_ptr<AudioFrame>)> callback) {
        callbackData = std::move(callback);
        this->ssrc = ssrc;
    }
} // wrtc