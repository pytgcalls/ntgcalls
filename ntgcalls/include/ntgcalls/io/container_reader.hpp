//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/threaded_reader.hpp>
#include <ntgcalls/media/containers/demuxer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::io {

    template<std::derived_from<media::containers::Demuxer> Demuxer>
    class ContainerReader: public ThreadedReader {
        Demuxer demuxer_;

        static Demuxer create_demuxer(media::BaseSink* sink) {
            if constexpr (std::constructible_from<Demuxer, media::BaseSink*>) {
                return Demuxer(sink);
            } else {
                return Demuxer();
            }
        }

        std::optional<media::containers::DemuxedFrame> pending_;
        std::chrono::nanoseconds last_duration_{0};
        bool eof_ = false;

        std::optional<media::containers::DemuxedFrame> next_frame() {
            while (true) {
                if (auto frame = demuxer_.next()) {
                    return frame;
                }
                if (eof_) {
                    return std::nullopt;
                }
                try {
                    const auto size = demuxer_.missing();
                    auto data = read(size);
                    demuxer_.feed(std::move(data), size);
                } catch (const EOFError&) {
                    eof_ = true;
                }
            }
        }

        std::chrono::nanoseconds frame_duration(const std::optional<media::containers::DemuxedFrame>& following) const {
            std::chrono::nanoseconds duration;
            if (pending_->timestamp && following && following->timestamp) {
                duration = std::chrono::nanoseconds(*following->timestamp - *pending_->timestamp);
            } else if (pending_->duration) {
                duration = *pending_->duration;
            } else {
                duration = last_duration_;
            }
            return duration > std::chrono::nanoseconds::zero() ? duration : sink_->frame_time();
        }

    protected:
        std::vector<Chunk> read_frames(const std::chrono::nanoseconds duration) override {
            std::vector<Chunk> frames;
            std::chrono::nanoseconds total{0};
            if (!pending_) {
                pending_ = next_frame();
            }
            while (pending_ && total < duration) {
                auto following = next_frame();
                const auto current_duration = frame_duration(following);
                last_duration_ = current_duration;
                frames.push_back({std::move(pending_->data), pending_->size, current_duration, pending_->frame_data});
                total += current_duration;
                pending_ = std::move(following);
            }
            if (frames.empty()) {
                RTC_LOG(LS_WARNING) << "Reached end of the container stream";
                throw EOFError("Reached end of the container stream");
            }
            return frames;
        }

    public:
        explicit ContainerReader(media::BaseSink* sink): BaseIO(sink), ThreadedReader(sink), demuxer_(create_demuxer(sink)) {}
    };

} // ntgcalls::io
