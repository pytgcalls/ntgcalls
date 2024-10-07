//
// Created by Laky64 on 07/10/24.
//

#include <wrtc/interfaces/media/remote_audio_sink.hpp>

namespace wrtc {
    RemoteAudioSink::RemoteAudioSink(const std::function<void(const std::vector<std::shared_ptr<AudioFrame>>&)>& callback): numSources(0) {
        framesCallback = callback;
    }

    RemoteAudioSink::~RemoteAudioSink() {
        audioFrames.clear();
    }

    void RemoteAudioSink::sendData(const std::shared_ptr<AudioFrame>& frame) {
        audioFrames.push_back(frame);
        if (audioFrames.size() >= numSources) {
            (void) framesCallback(audioFrames);
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