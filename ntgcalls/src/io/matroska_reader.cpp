//
// Created by Lauren on 20/11/26.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/matroska_reader.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::io {
    MatroskaReader::MatroskaReader(media::BaseSink* sink): BaseIO(sink), ThreadedReader(sink) {}

    std::vector<ThreadedReader::Chunk> MatroskaReader::read_frames(const std::chrono::nanoseconds duration) {
        std::vector<Chunk> frames;
        std::chrono::nanoseconds total{0};
        if (!pending_) {
            pending_ = next_frame();
        }
        while (pending_ && total < duration) {
            auto following = next_frame();
            const auto current_duration = frame_duration(following);
            last_duration_ = current_duration;
            const auto track = *demuxer_.track();
            frames.push_back({
                std::move(pending_->data),
                pending_->size,
                current_duration,
                {0, webrtc::kVideoRotation_0, track.width, track.height, track.codec, pending_->keyframe},
            });
            total += current_duration;
            pending_ = std::move(following);
        }
        if (frames.empty()) {
            RTC_LOG(LS_WARNING) << "Reached end of the Matroska stream";
            throw EOFError("Reached end of the Matroska stream");
        }
        return frames;
    }

    std::chrono::nanoseconds MatroskaReader::frame_duration(const std::optional<media::containers::MatroskaDemuxer::Frame>& following) const {
        std::chrono::nanoseconds duration{0};
        if (following) {
            duration = std::chrono::nanoseconds(following->timestamp - pending_->timestamp);
        } else if (const auto track = demuxer_.track(); track && track->default_duration > 0) {
            duration = std::chrono::nanoseconds(track->default_duration);
        } else {
            duration = last_duration_;
        }
        return duration > std::chrono::nanoseconds::zero() ? duration : sink_->frame_time();
    }

    std::optional<media::containers::MatroskaDemuxer::Frame> MatroskaReader::next_frame() {
        while (true) {
            if (auto frame = demuxer_.next()) {
                return frame;
            }
            if (eof_) {
                return std::nullopt;
            }
            try {
                const auto size = demuxer_.missing();
                const auto data = read(size);
                demuxer_.feed(data.get(), size);
            } catch (const EOFError&) {
                eof_ = true;
            }
        }
    }
} // ntgcalls::io
