//
// Created by Lauren on 20/11/26.
//

#include <ntgcalls/media/containers/matroska/matroska_muxer.hpp>

namespace ntgcalls::media::containers {
    MatroskaMuxer::MatroskaMuxer(const TrackInfo& track): codec_(track.codec), width_(track.width), height_(track.height) {}

    bool MatroskaMuxer::supports(const wrtc::models::Codec codec) {
        return codec == wrtc::models::Codec::H264 || codec == wrtc::models::Codec::VP8 || codec == wrtc::models::Codec::VP9;
    }

    bytes::binary MatroskaMuxer::finish() {
        return {};
    }

    bytes::binary MatroskaMuxer::add(const EncodedPacket& packet) {
        const auto* data = packet.data.data();
        const auto size = packet.data.size();
        const auto timestamp_ms = packet.timestamp_ms;
        const auto keyframe = packet.keyframe;
        bytes::binary output;
        if (!header_written_) {
            if (!keyframe) {
                return output;
            }
            bytes::binary codec_private;
            if (codec_ == wrtc::models::Codec::H264) {
                codec_private = avc_configuration(data, size);
                if (codec_private.empty()) {
                    return output;
                }
            }
            output = header(codec_private);
            header_written_ = true;
            first_timestamp_ = timestamp_ms;
        }
        const auto timestamp = timestamp_ms - *first_timestamp_;
        if (!cluster_timestamp_ || timestamp - *cluster_timestamp_ > kMaxClusterDuration || (keyframe && timestamp != *cluster_timestamp_) || timestamp < *cluster_timestamp_) {
            const auto next = cluster(timestamp);
            output.insert(output.end(), next.begin(), next.end());
            cluster_timestamp_ = timestamp;
        }
        const auto encoded = block(payload(data, size), timestamp - *cluster_timestamp_, keyframe);
        output.insert(output.end(), encoded.begin(), encoded.end());
        return output;
    }

    bytes::binary MatroskaMuxer::header(const bytes::binary& codec_private) const {
        bytes::binary ebml;
        for (const auto& child : {
                 uint_element(0x4286, 1),
                 uint_element(0x42F7, 1),
                 uint_element(0x42F2, 4),
                 uint_element(0x42F3, 8),
                 string_element(0x4282, "matroska"),
                 uint_element(0x4287, 4),
                 uint_element(0x4285, 2),
             }) {
            ebml.insert(ebml.end(), child.begin(), child.end());
        }
        auto output = element(0x1A45DFA3, ebml);
        append_id(output, 0x18538067);
        output.insert(output.end(), {0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
        bytes::binary info;
        for (const auto& child : {uint_element(0x2AD7B1, 1000000), string_element(0x4D80, "ntgcalls"), string_element(0x5741, "ntgcalls")}) {
            info.insert(info.end(), child.begin(), child.end());
        }
        const auto info_element = element(0x1549A966, info);
        output.insert(output.end(), info_element.begin(), info_element.end());
        bytes::binary video;
        for (const auto& child : {uint_element(0xB0, width_), uint_element(0xBA, height_)}) {
            video.insert(video.end(), child.begin(), child.end());
        }
        bytes::binary track;
        for (const auto& child : {uint_element(0xD7, 1), uint_element(0x73C5, 1), uint_element(0x83, 1), string_element(0x86, codec_id(codec_))}) {
            track.insert(track.end(), child.begin(), child.end());
        }
        if (!codec_private.empty()) {
            const auto private_element = element(0x63A2, codec_private);
            track.insert(track.end(), private_element.begin(), private_element.end());
        }
        const auto video_element = element(0xE0, video);
        track.insert(track.end(), video_element.begin(), video_element.end());
        const auto tracks = element(0x1654AE6B, element(0xAE, track));
        output.insert(output.end(), tracks.begin(), tracks.end());
        return output;
    }

    bytes::binary MatroskaMuxer::cluster(const int64_t timestamp) const {
        bytes::binary output;
        append_id(output, 0x1F43B675);
        output.insert(output.end(), {0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
        const auto timestamp_element = uint_element(0xE7, static_cast<uint64_t>(timestamp));
        output.insert(output.end(), timestamp_element.begin(), timestamp_element.end());
        return output;
    }

    bytes::binary MatroskaMuxer::block(const bytes::binary& payload, const int64_t relative, const bool keyframe) const {
        bytes::binary content = {0x81, static_cast<uint8_t>(relative >> 8 & 0xFF), static_cast<uint8_t>(relative & 0xFF), static_cast<uint8_t>(keyframe ? 0x80 : 0x00)};
        content.insert(content.end(), payload.begin(), payload.end());
        return element(0xA3, content);
    }

    bytes::binary MatroskaMuxer::payload(const uint8_t* data, const size_t size) const {
        if (codec_ != wrtc::models::Codec::H264) {
            return {data, data + size};
        }
        bytes::binary output;
        output.reserve(size + 16);
        for (const auto& [nal, length] : split_annex_b(data, size)) {
            for (int shift = 24; shift >= 0; shift -= 8) {
                output.push_back(static_cast<uint8_t>(length >> shift & 0xFF));
            }
            output.insert(output.end(), nal, nal + length);
        }
        return output;
    }

    std::vector<std::pair<const uint8_t*, size_t>> MatroskaMuxer::split_annex_b(const uint8_t* data, const size_t size) {
        std::vector<std::pair<const uint8_t*, size_t>> units;
        std::optional<size_t> start;
        size_t i = 0;
        while (i + 3 <= size) {
            if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) {
                if (start) {
                    auto end = i;
                    while (end > *start && data[end - 1] == 0) {
                        end--;
                    }
                    units.emplace_back(data + *start, end - *start);
                }
                start = i + 3;
                i += 3;
                continue;
            }
            i++;
        }
        if (start && *start < size) {
            units.emplace_back(data + *start, size - *start);
        }
        return units;
    }

    bytes::binary MatroskaMuxer::avc_configuration(const uint8_t* data, const size_t size) {
        const uint8_t* sps = nullptr;
        size_t sps_size = 0;
        const uint8_t* pps = nullptr;
        size_t pps_size = 0;
        for (const auto& [nal, length] : split_annex_b(data, size)) {
            if (length == 0) {
                continue;
            }
            if ((nal[0] & 0x1F) == 7 && !sps) {
                sps = nal;
                sps_size = length;
            } else if ((nal[0] & 0x1F) == 8 && !pps) {
                pps = nal;
                pps_size = length;
            }
        }
        if (!sps || !pps || sps_size < 4) {
            return {};
        }
        bytes::binary output = {1, sps[1], sps[2], sps[3], 0xFF, 0xE1, static_cast<uint8_t>(sps_size >> 8), static_cast<uint8_t>(sps_size & 0xFF)};
        output.insert(output.end(), sps, sps + sps_size);
        output.insert(output.end(), {1, static_cast<uint8_t>(pps_size >> 8), static_cast<uint8_t>(pps_size & 0xFF)});
        output.insert(output.end(), pps, pps + pps_size);
        return output;
    }

    std::string MatroskaMuxer::codec_id(const wrtc::models::Codec codec) {
        switch (codec) {
        case wrtc::models::Codec::VP8:
            return "V_VP8";
        case wrtc::models::Codec::VP9:
            return "V_VP9";
        default:
            return "V_MPEG4/ISO/AVC";
        }
    }

    bytes::binary MatroskaMuxer::element(const uint32_t id, const bytes::binary& payload) {
        bytes::binary output;
        append_id(output, id);
        append_size(output, payload.size());
        output.insert(output.end(), payload.begin(), payload.end());
        return output;
    }

    bytes::binary MatroskaMuxer::uint_element(const uint32_t id, const uint64_t value) {
        bytes::binary payload;
        int bytes_count = 1;
        while (bytes_count < 8 && value >> (bytes_count * 8)) {
            bytes_count++;
        }
        for (int i = bytes_count - 1; i >= 0; i--) {
            payload.push_back(static_cast<uint8_t>(value >> (i * 8) & 0xFF));
        }
        return element(id, payload);
    }

    bytes::binary MatroskaMuxer::string_element(const uint32_t id, const std::string& value) {
        return element(id, {value.begin(), value.end()});
    }

    void MatroskaMuxer::append_id(bytes::binary& output, const uint32_t id) {
        bool started = false;
        for (int shift = 24; shift >= 0; shift -= 8) {
            const auto byte = static_cast<uint8_t>(id >> shift & 0xFF);
            if (byte || started) {
                output.push_back(byte);
                started = true;
            }
        }
    }

    void MatroskaMuxer::append_size(bytes::binary& output, const uint64_t size) {
        int length = 1;
        while (length < 8 && size >= (1ULL << (7 * length)) - 1) {
            length++;
        }
        const uint64_t value = size | 1ULL << (7 * length);
        for (int i = length - 1; i >= 0; i--) {
            output.push_back(static_cast<uint8_t>(value >> (i * 8) & 0xFF));
        }
    }
} // ntgcalls::media::containers
