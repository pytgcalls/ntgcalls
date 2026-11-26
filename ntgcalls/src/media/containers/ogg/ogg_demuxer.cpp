//
// Created by Lauren on 20/11/26.
//

#include <algorithm>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/containers/ogg/ogg_demuxer.hpp>
#include <rtc_base/logging.h>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

namespace ntgcalls::media::containers {
    void OggDemuxer::feed(const bytes::unique_binary data, const size_t size) {
        buffer_.insert(buffer_.end(), data.get(), data.get() + size);
        while (parse_page()) {}
    }

    size_t OggDemuxer::page_size() const {
        const size_t segments = buffer_[26];
        size_t size = kPageHeaderSize + segments;
        for (size_t i = 0; i < segments; i++) {
            size += buffer_[kPageHeaderSize + i];
        }
        return size;
    }

    int64_t OggDemuxer::missing() const {
        if (buffer_.size() < kPageHeaderSize) {
            return static_cast<int64_t>(kPageHeaderSize - buffer_.size());
        }
        if (const size_t header_size = kPageHeaderSize + buffer_[26]; buffer_.size() < header_size) {
            return static_cast<int64_t>(header_size - buffer_.size());
        }
        return static_cast<int64_t>(std::max<size_t>(page_size() - buffer_.size(), 1));
    }

    bool OggDemuxer::parse_page() {
        if (buffer_.size() < kPageHeaderSize) {
            return false;
        }
        if (std::memcmp(buffer_.data(), "OggS", 4) != 0) {
            RTC_LOG(LS_ERROR) << "Invalid Ogg page";
            throw FileError("Invalid Ogg page, the input is not an Ogg Opus stream");
        }
        const size_t segments = buffer_[26];
        if (buffer_.size() < kPageHeaderSize + segments || buffer_.size() < page_size()) {
            return false;
        }
        const auto size = page_size();
        size_t offset = kPageHeaderSize + segments;
        size_t packet_start = offset;
        for (size_t i = 0; i < segments; i++) {
            const size_t length = buffer_[kPageHeaderSize + i];
            offset += length;
            if (length == 255) {
                continue;
            }
            if (discarding_) {
                discarding_ = false;
            } else if (partial_.empty()) {
                handle_packet(buffer_.data() + packet_start, offset - packet_start);
            } else {
                partial_.insert(partial_.end(), buffer_.begin() + static_cast<std::ptrdiff_t>(packet_start), buffer_.begin() + static_cast<std::ptrdiff_t>(offset));
                handle_packet(partial_.data(), partial_.size());
                partial_.clear();
            }
            packet_start = offset;
        }
        if (!discarding_) {
            partial_.insert(partial_.end(), buffer_.begin() + static_cast<std::ptrdiff_t>(packet_start), buffer_.begin() + static_cast<std::ptrdiff_t>(offset));
            if (partial_.size() > kMaxPacketSize) {
                RTC_LOG(LS_WARNING) << "Dropping an Ogg packet larger than " << kMaxPacketSize << " bytes";
                partial_.clear();
                discarding_ = true;
            }
        }
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(size));
        return true;
    }

    void OggDemuxer::handle_packet(const uint8_t* data, const size_t size) {
        if (size >= 19 && std::memcmp(data, "OpusHead", 8) == 0) {
            channels_ = data[9];
            return;
        }
        if (size >= 8 && std::memcmp(data, "OpusTags", 8) == 0) {
            return;
        }
        if (channels_ == 0) {
            RTC_LOG(LS_ERROR) << "The Ogg stream does not contain Opus audio";
            throw FileError("The Ogg stream does not contain Opus audio");
        }
        if (size > 0) {
            auto packet = bytes::make_unique_binary(size);
            std::memcpy(packet.get(), data, size);
            packets_.emplace_back(std::move(packet), size);
        }
    }

    std::optional<DemuxedFrame> OggDemuxer::next() {
        if (packets_.empty()) {
            return std::nullopt;
        }
        auto [data, size] = std::move(packets_.front());
        packets_.pop_front();
        const auto duration = std::chrono::milliseconds(10) * std::max<size_t>(wrtc::audio_factory::OpusPacket::frames(data.get(), size), 1);
        return DemuxedFrame{
            std::move(data),
            size,
            std::nullopt,
            duration,
            {0, webrtc::kVideoRotation_0, 0, 0, wrtc::models::Codec::Opus, true},
        };
    }

    uint8_t OggDemuxer::channels() const {
        return channels_;
    }
} // ntgcalls::media::containers
