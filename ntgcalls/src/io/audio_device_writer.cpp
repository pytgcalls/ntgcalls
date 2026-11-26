//
// Created by Lauren on 07/10/24.
//

#include <ntgcalls/io/audio_device_writer.hpp>
#include <ntgcalls/media/mixers/audio_mixer.hpp>

namespace ntgcalls::io {
    AudioDeviceWriter::AudioDeviceWriter(media::BaseSink* sink): BaseWriter(sink) {}

    void AudioDeviceWriter::send_frames(const std::vector<wrtc::models::Frame> frames) {
        if (!sink_) return;
        const auto frame_size = sink_->frame_size();
        on_data(media::mixers::AudioMixer::mix_pcm(frames, frame_size), frame_size);
    }
} // ntgcalls::io
