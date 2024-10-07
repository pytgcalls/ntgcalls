//
// Created by Laky64 on 07/10/24.
//

#include <cstring>
#include <wrtc/interfaces/media/raw_audio_sink.hpp>

namespace wrtc {
    void RawAudioSink::OnData(const Data& audio) {
        if (remoteAudioSink) {
            const auto frame = std::make_shared<AudioFrame>(ssrc);
            const size_t dataSize = audio.samples_per_channel * audio.channels * sizeof(int16_t);
            frame->size = dataSize;
            frame->data = bytes::make_unique_binary(dataSize);
            memcpy(frame->data.get(), audio.data, dataSize);
            remoteAudioSink->sendData(frame);
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