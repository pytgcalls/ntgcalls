//
// Created by Laky64 on 12/08/2023.
//

#include <ntgcalls/media/audio_streamer.hpp>

namespace ntgcalls {
    AudioStreamer::AudioStreamer() {
        audio = std::make_unique<wrtc::RTCAudioSource>();
    }

    AudioStreamer::~AudioStreamer() {
        audio = nullptr;
    }

    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> AudioStreamer::createTrack() {
        return audio->createTrack();
    }

    void AudioStreamer::sendData(uint8_t* sample, const wrtc::FrameData additionalData) {
        frames++;
        auto event = wrtc::RTCOnDataEvent(sample, frameSize() / (2 * description->channelCount));
        event.channelCount = description->channelCount;
        event.sampleRate = description->sampleRate;
        event.bitsPerSample = 16;
        audio->OnData(event, additionalData);
    }
}
