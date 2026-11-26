//
// Created by Lauren on 26/11/26.
//

#include <algorithm>
#include <ntgcalls/io/threaded_reader.hpp>
#include <ntgcalls/media/containers/raw/raw_demuxer.hpp>

namespace ntgcalls::media::containers {
    RawDemuxer::RawDemuxer(BaseSink* sink):
    frame_size_(static_cast<size_t>(sink->frame_size())),
    frame_time_(sink->frame_time()),
    batch_size_(frame_size_ * static_cast<size_t>(std::max<int64_t>(1, io::ThreadedReader::kBufferDuration / frame_time_))) {}

    std::optional<DemuxedFrame> RawDemuxer::next(ByteSource& source) {
        if (!chunk_ || offset_ + frame_size_ > chunk_size_) {
            chunk_ = source.read(static_cast<int64_t>(batch_size_));
            chunk_size_ = batch_size_;
            offset_ = 0;
        }
        auto frame = bytes::make_unique_binary(chunk_.get() + offset_, frame_size_);
        offset_ += frame_size_;
        return DemuxedFrame{std::move(frame), frame_size_, std::nullopt, frame_time_, {}};
    }
} // ntgcalls::media::containers
