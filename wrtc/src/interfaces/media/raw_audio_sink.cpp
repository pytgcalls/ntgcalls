//
// Created by Laky64 on 07/10/24.
//

#include <wrtc/interfaces/media/raw_audio_sink.hpp>

namespace wrtc {
    void RawAudioSink::OnData(const Data& audio) {
        if (remoteAudioSink) {
            auto frame = std::make_unique<AudioFrame>(ssrc);
            frame->size = audio.samples_per_channel * audio.channels * sizeof(int16_t);
            frame->data = audio.data;
            frame->sampleRate = audio.sample_rate;
            frame->channels = audio.channels;
            remoteAudioSink->sendData(std::move(frame));
        }
    }

    RawAudioSink::~RawAudioSink() {
        remoteAudioSink = nullptr;
    }

    void RawAudioSink::setRemoteAudioSink(const uint32_t ssrc, RemoteAudioSink* sink) {
        remoteAudioSink = sink;
        this->ssrc = ssrc;
    }
} // wrtc