//
// Created by Lauren on 19/11/26.
//

#include <cstring>
#include <ntgcalls/io/raw_reader.hpp>

namespace ntgcalls::io {
    RawReader::RawReader(media::BaseSink* sink): BaseIO(sink), ThreadedReader(sink) {}

    std::vector<ThreadedReader::Chunk> RawReader::read_frames(const std::chrono::nanoseconds duration) {
        const auto frame_time = sink_->frame_time();
        const auto count = static_cast<size_t>(std::max<int64_t>(1, duration / frame_time));
        const auto frame_size = sink_->frame_size();
        const auto data = read(frame_size * static_cast<int64_t>(count));
        std::vector<Chunk> frames;
        frames.reserve(count);
        for (size_t i = 0; i < count; i++) {
            auto chunk = bytes::make_unique_binary(frame_size);
            std::memcpy(chunk.get(), data.get() + i * frame_size, frame_size);
            frames.push_back({std::move(chunk), static_cast<size_t>(frame_size), frame_time, {}});
        }
        return frames;
    }
} // ntgcalls::io
