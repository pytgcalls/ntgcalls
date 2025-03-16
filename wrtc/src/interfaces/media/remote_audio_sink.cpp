//
// Created by Laky64 on 07/10/24.
//

#include <wrtc/interfaces/media/remote_audio_sink.hpp>

namespace wrtc {
    RemoteAudioSink::RemoteAudioSink(const std::function<void(const std::vector<std::unique_ptr<AudioFrame>>&)>& callback): numSources(0) {
        framesCallback = callback;
    }

    RemoteAudioSink::~RemoteAudioSink() {
        audioFrames.clear();
    }

    void RemoteAudioSink::sendData(std::unique_ptr<AudioFrame> frame) {
        audioFrames.push_back(std::move(frame));
        if (audioFrames.size() >= numSources) {
            framesCallback(audioFrames);
            audioFrames.clear();
        }
    }

    void RemoteAudioSink::addSource() {
        ++numSources;
    }

    void RemoteAudioSink::removeSource() {
        --numSources;
    }
} // wrtc