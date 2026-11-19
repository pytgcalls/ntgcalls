//
// Created by Lauren on 20/11/26.
//

#include <cstring>
#include <ntgcalls/io/audio_mixer.hpp>
#include <ntgcalls/io/raw_writer.hpp>

namespace ntgcalls::io {
    RawWriter::RawWriter(media::BaseSink* sink): BaseIO(sink), ThreadedWriter(sink) {}

    void RawWriter::send_frames(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) {
        const auto frame_size = sink_->frame_size();
        push(AudioMixer::mix(frames, frame_size), frame_size);
    }

    std::optional<std::pair<bytes::unique_binary, size_t>> RawWriter::idle_data() {
        const auto frame_size = sink_->frame_size();
        auto silence = bytes::make_unique_binary(frame_size);
        std::memset(silence.get(), 0, frame_size);
        return std::pair{std::move(silence), static_cast<size_t>(frame_size)};
    }
} // ntgcalls::io
