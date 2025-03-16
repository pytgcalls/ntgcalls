//
// Created by Laky64 on 12/08/2023.
//

#pragma once

#include <wrtc/wrtc.hpp>
#include <ntgcalls/media/audio_sink.hpp>
#include <ntgcalls/media/base_streamer.hpp>

namespace ntgcalls {
    class AudioStreamer final : public AudioSink, public BaseStreamer {
        std::unique_ptr<wrtc::RTCAudioSource> audio;

    public:
        AudioStreamer();

        ~AudioStreamer() override;

        rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> createTrack() override;

        void sendData(uint8_t* sample, wrtc::FrameData additionalData) override;
    };
}
