//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <ntgcalls/io/threaded_reader.hpp>
#include <ntgcalls/media/containers/matroska/matroska_demuxer.hpp>

namespace ntgcalls::io {

    class MatroskaReader: public ThreadedReader {
        media::containers::MatroskaDemuxer demuxer_;
        std::optional<media::containers::MatroskaDemuxer::Frame> pending_;
        std::chrono::nanoseconds last_duration_{0};
        bool eof_ = false;

        std::optional<media::containers::MatroskaDemuxer::Frame> next_frame();

        std::chrono::nanoseconds frame_duration(const std::optional<media::containers::MatroskaDemuxer::Frame>& following) const;

    protected:
        std::vector<Chunk> read_frames(std::chrono::nanoseconds duration) override;

    public:
        explicit MatroskaReader(media::BaseSink* sink);
    };

} // ntgcalls::io
