//
// Created by Lauren on 20/11/26.
//

#include <string>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/containers/matroska/matroska_demuxer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::media::containers {
    void MatroskaDemuxer::feed(const bytes::unique_binary data, const size_t size) {
        buffer_.insert(buffer_.end(), data.get(), data.get() + size);
        while (true) {
            if (skip_ > 0) {
                const auto count = std::min<uint64_t>(skip_, buffer_.size());
                buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(count));
                skip_ -= count;
                if (skip_ > 0) {
                    break;
                }
            }
            if (!parse_element()) {
                break;
            }
        }
    }

    int64_t MatroskaDemuxer::missing() const {
        if (skip_ > 0) {
            return static_cast<int64_t>(std::min(skip_, kMaxSkipChunk));
        }
        if (const auto element = peek_element()) {
            return static_cast<int64_t>(std::max<uint64_t>(element->header_size + element->size - buffer_.size(), 1));
        }
        if (buffer_.empty()) {
            return 1;
        }
        const auto id = read_vint(buffer_.data(), buffer_.size(), true);
        if (!id) {
            return 1;
        }
        if (buffer_.size() <= id->second) {
            return static_cast<int64_t>(id->second + 1 - buffer_.size());
        }
        size_t length = 1;
        for (uint8_t mask = 0x80; length <= 8 && !(buffer_[id->second] & mask); mask >>= 1) {
            ++length;
        }
        return static_cast<int64_t>(std::max<size_t>(id->second + length - buffer_.size(), 1));
    }

    std::optional<MatroskaDemuxer::Element> MatroskaDemuxer::peek_element() const {
        const auto id = read_vint(buffer_.data(), buffer_.size(), true);
        if (!id) {
            return std::nullopt;
        }
        const auto size = read_vint(buffer_.data() + id->second, buffer_.size() - id->second, false);
        if (!size) {
            return std::nullopt;
        }
        return Element{static_cast<uint32_t>(id->first), size->first, id->second + size->second};
    }

    bool MatroskaDemuxer::parse_element() {
        const auto element = peek_element();
        if (!element) {
            return false;
        }
        if (!header_checked_) {
            if (element->id != kEbmlId) {
                RTC_LOG(LS_ERROR) << "The input is not a Matroska stream";
                throw FileError("The input is not a Matroska stream");
            }
            header_checked_ = true;
        }
        const auto& [id, size, header_size] = *element;
        if (id == kSegmentId || id == kClusterId) {
            buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(header_size));
            return true;
        }
        if (size == kUnknownSize) {
            RTC_LOG(LS_ERROR) << "Unsupported Matroska element with an unknown size";
            throw FileError("Unsupported Matroska element with an unknown size");
        }
        if (id != kInfoId && id != kTracksId && id != kTimestampId && id != kSimpleBlockId && id != kBlockGroupId) {
            buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(header_size));
            skip_ = size;
            return true;
        }
        if (size > kMaxElementSize) {
            RTC_LOG(LS_ERROR) << "Matroska element of " << size << " bytes exceeds the limit of " << kMaxElementSize << " bytes";
            throw FileError("Invalid Matroska stream, an element is too large");
        }
        if (buffer_.size() < header_size + size) {
            return false;
        }
        const auto* payload = buffer_.data() + header_size;
        if (id == kInfoId) {
            parse_info(payload, size);
        } else if (id == kTracksId) {
            parse_tracks(payload, size);
        } else if (id == kTimestampId) {
            cluster_timestamp_ = static_cast<int64_t>(read_uint(payload, size));
        } else if (id == kSimpleBlockId) {
            parse_block(payload, size, std::nullopt);
        } else {
            parse_block_group(payload, size);
        }
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(header_size + size));
        return true;
    }

    void MatroskaDemuxer::parse_info(const uint8_t* data, const size_t size) {
        size_t offset = 0;
        while (offset < size) {
            const auto id = read_vint(data + offset, size - offset, true);
            const auto length = id ? read_vint(data + offset + id->second, size - offset - id->second, false) : std::nullopt;
            if (!id || !length || length->first > size - offset - id->second - length->second) {
                return;
            }
            const auto* payload = data + offset + id->second + length->second;
            if (id->first == kTimestampScaleId) {
                timestamp_scale_ = read_uint(payload, length->first);
            }
            offset += id->second + length->second + length->first;
        }
    }

    void MatroskaDemuxer::parse_tracks(const uint8_t* data, const size_t size) {
        size_t offset = 0;
        while (offset < size && !track_) {
            const auto id = read_vint(data + offset, size - offset, true);
            const auto length = id ? read_vint(data + offset + id->second, size - offset - id->second, false) : std::nullopt;
            if (!id || !length || length->first > size - offset - id->second - length->second) {
                return;
            }
            if (id->first == kTrackEntryId) {
                parse_track_entry(data + offset + id->second + length->second, length->first);
            }
            offset += id->second + length->second + length->first;
        }
        if (!track_) {
            RTC_LOG(LS_ERROR) << "The Matroska stream does not contain a video track";
            throw FileError("The Matroska stream does not contain a video track");
        }
    }

    void MatroskaDemuxer::parse_track_entry(const uint8_t* data, const size_t size) {
        Track track;
        uint64_t type = 0;
        std::string codec_id;
        bytes::binary codec_private;
        size_t offset = 0;
        while (offset < size) {
            const auto id = read_vint(data + offset, size - offset, true);
            const auto length = id ? read_vint(data + offset + id->second, size - offset - id->second, false) : std::nullopt;
            if (!id || !length || length->first > size - offset - id->second - length->second) {
                return;
            }
            const auto* payload = data + offset + id->second + length->second;
            const auto payload_size = static_cast<size_t>(length->first);
            if (id->first == kTrackNumberId) {
                track.number = read_uint(payload, payload_size);
            } else if (id->first == kTrackTypeId) {
                type = read_uint(payload, payload_size);
            } else if (id->first == kCodecId) {
                codec_id.assign(reinterpret_cast<const char*>(payload), payload_size);
            } else if (id->first == kCodecPrivateId) {
                codec_private.assign(payload, payload + payload_size);
            } else if (id->first == kDefaultDurationId) {
                track.default_duration = read_uint(payload, payload_size);
            } else if (id->first == kVideoId) {
                size_t video_offset = 0;
                while (video_offset < payload_size) {
                    const auto video_id = read_vint(payload + video_offset, payload_size - video_offset, true);
                    const auto video_length = video_id ? read_vint(payload + video_offset + video_id->second, payload_size - video_offset - video_id->second, false) : std::nullopt;
                    if (!video_id || !video_length || video_length->first > payload_size - video_offset - video_id->second - video_length->second) {
                        break;
                    }
                    const auto* value = payload + video_offset + video_id->second + video_length->second;
                    if (video_id->first == kPixelWidthId) {
                        track.width = static_cast<uint16_t>(read_uint(value, video_length->first));
                    } else if (video_id->first == kPixelHeightId) {
                        track.height = static_cast<uint16_t>(read_uint(value, video_length->first));
                    }
                    video_offset += video_id->second + video_length->second + video_length->first;
                }
            }
            offset += id->second + length->second + length->first;
        }
        if (type != 1) {
            return;
        }
        track.codec = codec_from_id(codec_id);
        track_ = track;
        parse_codec_private(codec_private);
    }

    void MatroskaDemuxer::parse_codec_private(const bytes::binary& codec_private) {
        if (track_->codec == wrtc::models::Codec::H264 && codec_private.size() >= 7) {
            nal_length_size_ = (codec_private[4] & 0x03) + 1;
            size_t offset = 5;
            for (int group = 0; group < 2 && offset < codec_private.size(); group++) {
                const size_t count = group == 0 ? codec_private[offset] & 0x1F : codec_private[offset];
                ++offset;
                for (size_t i = 0; i < count && offset + 2 <= codec_private.size(); i++) {
                    constexpr uint8_t kStartCode[] = {0, 0, 0, 1};
                    const size_t length = codec_private[offset] << 8 | codec_private[offset + 1];
                    offset += 2;
                    if (offset + length > codec_private.size()) {
                        return;
                    }
                    parameter_sets_.insert(parameter_sets_.end(), std::begin(kStartCode), std::end(kStartCode));
                    parameter_sets_.insert(parameter_sets_.end(), codec_private.begin() + static_cast<std::ptrdiff_t>(offset), codec_private.begin() + static_cast<std::ptrdiff_t>(offset + length));
                    offset += length;
                }
            }
        } else if (track_->codec == wrtc::models::Codec::AV1 && codec_private.size() > 4) {
            parameter_sets_.assign(codec_private.begin() + 4, codec_private.end());
        }
    }

    void MatroskaDemuxer::parse_block_group(const uint8_t* data, const size_t size) {
        const uint8_t* block = nullptr;
        size_t block_size = 0;
        bool referenced = false;
        size_t offset = 0;
        while (offset < size) {
            const auto id = read_vint(data + offset, size - offset, true);
            const auto length = id ? read_vint(data + offset + id->second, size - offset - id->second, false) : std::nullopt;
            if (!id || !length || length->first > size - offset - id->second - length->second) {
                break;
            }
            if (id->first == kBlockId) {
                block = data + offset + id->second + length->second;
                block_size = length->first;
            } else if (id->first == kReferenceBlockId) {
                referenced = true;
            }
            offset += id->second + length->second + length->first;
        }
        if (block) {
            parse_block(block, block_size, !referenced);
        }
    }

    void MatroskaDemuxer::parse_block(const uint8_t* data, const size_t size, const std::optional<bool> keyframe) {
        const auto track_number = read_vint(data, size, false);
        if (!track_ || !track_number || track_number->first != track_->number || size < track_number->second + 3) {
            return;
        }
        const auto* header = data + track_number->second;
        const auto relative = static_cast<int16_t>(header[0] << 8 | header[1]);
        const auto flags = header[2];
        if (flags >> 1 & 0x03) {
            RTC_LOG(LS_ERROR) << "Laced Matroska video blocks are not supported";
            throw FileError("Laced Matroska video blocks are not supported");
        }
        const auto timestamp = (cluster_timestamp_ + relative) * static_cast<int64_t>(timestamp_scale_);
        if (timestamp < last_timestamp_) {
            RTC_LOG(LS_ERROR) << "The video uses B-frames, which can't be sent without re-encoding";
            throw FileError("The video uses B-frames, which can't be sent without re-encoding");
        }
        last_timestamp_ = timestamp;
        const bool is_keyframe = keyframe.value_or((flags & 0x80) != 0);
        const auto offset = track_number->second + 3;
        auto payload = convert_payload(data + offset, size - offset, is_keyframe);
        auto frame = bytes::make_unique_binary(payload.size());
        std::memcpy(frame.get(), payload.data(), payload.size());
        frames_.push_back({std::move(frame), payload.size(), timestamp, is_keyframe});
    }

    bytes::binary MatroskaDemuxer::convert_payload(const uint8_t* data, const size_t size, const bool keyframe) const {
        if (track_->codec == wrtc::models::Codec::H264) {
            bytes::binary annex_b;
            annex_b.reserve(size + parameter_sets_.size() + 16);
            size_t offset = 0;
            while (offset + nal_length_size_ <= size) {
                constexpr uint8_t kStartCode[] = {0, 0, 0, 1};
                size_t length = 0;
                for (size_t i = 0; i < nal_length_size_; i++) {
                    length = length << 8 | data[offset + i];
                }
                offset += nal_length_size_;
                if (offset + length > size) {
                    break;
                }
                annex_b.insert(annex_b.end(), std::begin(kStartCode), std::end(kStartCode));
                annex_b.insert(annex_b.end(), data + offset, data + offset + length);
                offset += length;
            }
            if (keyframe && !has_nal_type(annex_b, 7)) {
                annex_b.insert(annex_b.begin(), parameter_sets_.begin(), parameter_sets_.end());
            }
            return annex_b;
        }
        bytes::binary payload;
        if (track_->codec == wrtc::models::Codec::AV1 && keyframe && !has_obu_type(data, size, 1)) {
            payload = parameter_sets_;
        }
        payload.insert(payload.end(), data, data + size);
        return payload;
    }

    std::optional<std::pair<uint64_t, size_t>> MatroskaDemuxer::read_vint(const uint8_t* data, const size_t size, const bool keep_marker) {
        if (size == 0) {
            return std::nullopt;
        }
        size_t length = 1;
        uint8_t mask = 0x80;
        while (length <= 8 && !(data[0] & mask)) {
            mask >>= 1;
            ++length;
        }
        if (length > 8) {
            RTC_LOG(LS_ERROR) << "Invalid Matroska variable size integer";
            throw FileError("Invalid Matroska stream");
        }
        if (size < length) {
            return std::nullopt;
        }
        uint64_t value = keep_marker ? data[0] : data[0] & (mask - 1);
        for (size_t i = 1; i < length; i++) {
            value = value << 8 | data[i];
        }
        if (!keep_marker && value == (1ULL << (7 * length)) - 1) {
            return std::pair{kUnknownSize, length};
        }
        return std::pair{value, length};
    }

    uint64_t MatroskaDemuxer::read_uint(const uint8_t* data, const size_t size) {
        uint64_t value = 0;
        for (size_t i = 0; i < size && i < 8; i++) {
            value = value << 8 | data[i];
        }
        return value;
    }

    wrtc::models::Codec MatroskaDemuxer::codec_from_id(const std::string& codec_id) {
        if (codec_id == "V_MPEG4/ISO/AVC") {
            return wrtc::models::Codec::H264;
        }
        if (codec_id == "V_VP8") {
            return wrtc::models::Codec::VP8;
        }
        if (codec_id == "V_VP9") {
            return wrtc::models::Codec::VP9;
        }
        if (codec_id == "V_AV1") {
            return wrtc::models::Codec::AV1;
        }
        RTC_LOG(LS_ERROR) << "Unsupported Matroska video codec " << codec_id;
        throw FileError("Unsupported Matroska video codec " + codec_id + ", only H264, VP8, VP9 and AV1 can be sent without re-encoding");
    }

    bool MatroskaDemuxer::has_nal_type(const bytes::binary& annex_b, const uint8_t type) {
        for (size_t i = 0; i + 3 < annex_b.size(); i++) {
            if (annex_b[i] == 0 && annex_b[i + 1] == 0 && annex_b[i + 2] == 1 && (annex_b[i + 3] & 0x1F) == type) {
                return true;
            }
        }
        return false;
    }

    bool MatroskaDemuxer::has_obu_type(const uint8_t* data, const size_t size, const uint8_t type) {
        size_t offset = 0;
        while (offset < size) {
            const auto header = data[offset];
            if ((header >> 3 & 0x0F) == type) {
                return true;
            }
            offset += header & 0x04 ? 2 : 1;
            if (!(header & 0x02)) {
                return false;
            }
            uint64_t length = 0;
            for (int i = 0; i < 8 && offset < size; i++) {
                const auto byte = data[offset++];
                length |= static_cast<uint64_t>(byte & 0x7F) << (i * 7);
                if (!(byte & 0x80)) {
                    break;
                }
            }
            offset += length;
        }
        return false;
    }

    std::optional<DemuxedFrame> MatroskaDemuxer::next() {
        if (frames_.empty() || !track_) {
            return std::nullopt;
        }
        auto [data, size, timestamp, keyframe] = std::move(frames_.front());
        frames_.pop_front();
        std::optional<std::chrono::nanoseconds> duration;
        if (track_->default_duration > 0) {
            duration = std::chrono::nanoseconds(track_->default_duration);
        }
        return DemuxedFrame{
            std::move(data),
            size,
            timestamp,
            duration,
            {0, webrtc::kVideoRotation_0, track_->width, track_->height, track_->codec, keyframe},
        };
    }

    std::optional<MatroskaDemuxer::Track> MatroskaDemuxer::track() const {
        return track_;
    }
} // ntgcalls::media::containers
