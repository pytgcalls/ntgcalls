//
// Created by Lauren on 19/11/26.
//

#pragma once

#include <ntgcalls/io/threaded_reader.hpp>
#include <ntgcalls/media/containers/ogg/ogg_demuxer.hpp>

namespace ntgcalls::io {

    class OggReader: public ThreadedReader {
        media::containers::OggDemuxer demuxer_;
        bool eof_ = false;

        std::optional<std::pair<bytes::unique_binary, size_t>> next_packet();

    protected:
        std::vector<Chunk> read_frames(std::chrono::nanoseconds duration) override;

    public:
        explicit OggReader(media::BaseSink* sink);
    };

} // ntgcalls::io
