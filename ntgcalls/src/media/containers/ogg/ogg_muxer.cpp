//
// Created by Lauren on 20/11/26.
//

#include <random>
#include <string_view>
#include <ntgcalls/media/containers/ogg/ogg_muxer.hpp>

namespace ntgcalls::media::containers {
    OggMuxer::OggMuxer(const uint8_t channels): channels_(channels), serial_(std::random_device{}()) {}

    std::vector<bytes::binary> OggMuxer::add(bytes::binary packet, const int samples) {
        std::vector<bytes::binary> pages;
        if (packet.empty()) {
            return pages;
        }
        write_headers(pages);
        const auto packet_segments = segments(packet.size());
        if (packets_.size() >= kMaxPagePackets || segments_ + packet_segments > kMaxPageSegments) {
            pages.push_back(page(packets_, 0));
            packets_.clear();
            segments_ = 0;
        }
        granule_ += samples;
        segments_ += packet_segments;
        packets_.push_back(std::move(packet));
        return pages;
    }

    std::vector<bytes::binary> OggMuxer::finish() {
        std::vector<bytes::binary> pages;
        if (!headers_written_) {
            return pages;
        }
        pages.push_back(page(packets_, 0x04));
        packets_.clear();
        segments_ = 0;
        return pages;
    }

    uint64_t OggMuxer::granule() const {
        return granule_;
    }

    void OggMuxer::write_headers(std::vector<bytes::binary>& pages) {
        if (headers_written_) {
            return;
        }
        headers_written_ = true;
        const bytes::binary head = {'O', 'p', 'u', 's', 'H', 'e', 'a', 'd', 1, channels_, 0x38, 0x01, 0x80, 0xBB, 0x00, 0x00, 0, 0, 0};
        pages.push_back(page({head}, 0x02));
        constexpr std::string_view kVendor = "ntgcalls";
        bytes::binary tags = {'O', 'p', 'u', 's', 'T', 'a', 'g', 's', static_cast<uint8_t>(kVendor.size()), 0, 0, 0};
        tags.insert(tags.end(), kVendor.begin(), kVendor.end());
        tags.insert(tags.end(), {0, 0, 0, 0});
        pages.push_back(page({tags}, 0));
    }

    bytes::binary OggMuxer::page(const std::vector<bytes::binary>& packets, const uint8_t flags) {
        const auto granule = flags & 0x02 ? 0 : granule_;
        bytes::binary data = {'O', 'g', 'g', 'S', 0, flags};
        for (int i = 0; i < 8; i++) {
            data.push_back(static_cast<uint8_t>(granule >> (i * 8)));
        }
        for (const auto value : {serial_, sequence_++}) {
            for (int i = 0; i < 4; i++) {
                data.push_back(static_cast<uint8_t>(value >> (i * 8)));
            }
        }
        data.insert(data.end(), {0, 0, 0, 0});
        size_t total = 0;
        for (const auto& packet : packets) {
            total += segments(packet.size());
        }
        data.push_back(static_cast<uint8_t>(total));
        for (const auto& packet : packets) {
            auto size = packet.size();
            while (size >= 255) {
                data.push_back(255);
                size -= 255;
            }
            data.push_back(static_cast<uint8_t>(size));
        }
        for (const auto& packet : packets) {
            data.insert(data.end(), packet.begin(), packet.end());
        }
        const auto checksum = crc(data);
        for (int i = 0; i < 4; i++) {
            data[22 + i] = static_cast<uint8_t>(checksum >> (i * 8));
        }
        return data;
    }

    size_t OggMuxer::segments(const size_t size) {
        return size / 255 + 1;
    }

    std::array<uint32_t, 256> OggMuxer::crc_table() {
        std::array<uint32_t, 256> values{};
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t value = i << 24;
            for (int j = 0; j < 8; j++) {
                value = value & 0x80000000 ? value << 1 ^ 0x04C11DB7 : value << 1;
            }
            values[i] = value;
        }
        return values;
    }

    uint32_t OggMuxer::crc(const bytes::binary& page) {
        static const auto kTable = crc_table();
        uint32_t value = 0;
        for (const auto byte : page) {
            value = value << 8 ^ kTable[(value >> 24 & 0xFF) ^ byte];
        }
        return value;
    }
} // ntgcalls::media::containers
