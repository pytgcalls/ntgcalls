//
// Created by Lauren on 20/11/26.
//

#include <cstring>
#include <ntgcalls/io/audio_mixer.hpp>
#include <ntgcalls/io/raw_writer.hpp>
#include <ntgcalls/media/video_sink.hpp>

namespace ntgcalls::io {
    RawWriter::RawWriter(media::BaseSink* sink): BaseIO(sink), ThreadedWriter(sink) {
        if (const auto video_sink = dynamic_cast<media::VideoSink*>(sink)) {
            if (const auto description = video_sink->get_config()) {
                composer_ = std::make_unique<media::composer::VideoComposer>(description->width, description->height);
            }
        }
    }

    void RawWriter::send_frames(std::vector<wrtc::models::Frame> frames) {
        if (composer_) {
            for (auto& frame : frames) {
                composer_->update(frame.ssrc, std::move(frame.data), frame.frame_data.width, frame.frame_data.height, frame.frame_data.rotation);
            }
            return;
        }
        const auto frame_size = sink_->frame_size();
        push(AudioMixer::mix(frames, frame_size), frame_size);
    }

    void RawWriter::remove_source(const int64_t ssrc) {
        if (composer_) {
            composer_->remove(ssrc);
        }
    }

    std::optional<std::pair<bytes::unique_binary, size_t>> RawWriter::idle_data() {
        if (composer_) {
            return std::pair{composer_->compose(), composer_->frame_size()};
        }
        const auto frame_size = sink_->frame_size();
        auto silence = bytes::make_unique_binary(frame_size);
        std::memset(silence.get(), 0, frame_size);
        return std::pair{std::move(silence), static_cast<size_t>(frame_size)};
    }

    std::chrono::nanoseconds RawWriter::idle_interval() const {
        return composer_ ? sink_->frame_time() : ThreadedWriter::idle_interval();
    }
} // ntgcalls::io
