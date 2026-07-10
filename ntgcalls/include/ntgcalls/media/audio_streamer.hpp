//
// Created by Lauren on 12/08/23.
//

#pragma once

#include <ntgcalls/media/audio_sink.hpp>
#include <ntgcalls/media/base_streamer.hpp>
#include <wrtc/interfaces/media/rtc_audio_source.hpp>

namespace ntgcalls::media {
    class AudioStreamer final : public AudioSink, public BaseStreamer {
        std::unique_ptr<wrtc::interfaces::media::RTCAudioSource> audio_;

    public:
        AudioStreamer();

        ~AudioStreamer() override;

        webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> createTrack() override;

        void sendData(uint8_t* sample, size_t size, wrtc::models::FrameData additional_data) override;
    };
} // ntgcalls::media
