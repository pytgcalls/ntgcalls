//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <deque>
#include <optional>
#include <wrtc/models/codec.hpp>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::media::containers {

    class MatroskaDemuxer {
    public:
        struct Track {
            uint64_t number = 0;
            wrtc::models::Codec codec = wrtc::models::Codec::H264;
            uint16_t width = 0;
            uint16_t height = 0;
            uint64_t default_duration = 0;
        };

        struct Frame {
            bytes::unique_binary data;
            size_t size = 0;
            int64_t timestamp = 0;
            bool keyframe = false;
        };

    private:
        static constexpr uint32_t kEbmlId = 0x1A45DFA3;
        static constexpr uint32_t kSegmentId = 0x18538067;
        static constexpr uint32_t kInfoId = 0x1549A966;
        static constexpr uint32_t kTimestampScaleId = 0x2AD7B1;
        static constexpr uint32_t kTracksId = 0x1654AE6B;
        static constexpr uint32_t kTrackEntryId = 0xAE;
        static constexpr uint32_t kTrackNumberId = 0xD7;
        static constexpr uint32_t kTrackTypeId = 0x83;
        static constexpr uint32_t kCodecId = 0x86;
        static constexpr uint32_t kCodecPrivateId = 0x63A2;
        static constexpr uint32_t kDefaultDurationId = 0x23E383;
        static constexpr uint32_t kVideoId = 0xE0;
        static constexpr uint32_t kPixelWidthId = 0xB0;
        static constexpr uint32_t kPixelHeightId = 0xBA;
        static constexpr uint32_t kClusterId = 0x1F43B675;
        static constexpr uint32_t kTimestampId = 0xE7;
        static constexpr uint32_t kSimpleBlockId = 0xA3;
        static constexpr uint32_t kBlockGroupId = 0xA0;
        static constexpr uint32_t kBlockId = 0xA1;
        static constexpr uint32_t kReferenceBlockId = 0xFB;
        static constexpr uint64_t kUnknownSize = UINT64_MAX;
        static constexpr uint64_t kMaxSkipChunk = 65536;

        struct Element {
            uint32_t id = 0;
            uint64_t size = 0;
            size_t header_size = 0;
        };

        bytes::binary buffer_;
        uint64_t skip_ = 0;
        bool header_checked_ = false;
        uint64_t timestamp_scale_ = 1000000;
        int64_t cluster_timestamp_ = 0;
        int64_t last_timestamp_ = INT64_MIN;
        std::optional<Track> track_;
        uint8_t nal_length_size_ = 4;
        bytes::binary parameter_sets_;
        std::deque<Frame> frames_;

        bool parse_element();

        std::optional<Element> peek_element() const;

        void parse_info(const uint8_t* data, size_t size);

        void parse_tracks(const uint8_t* data, size_t size);

        void parse_track_entry(const uint8_t* data, size_t size);

        void parse_codec_private(const bytes::binary& codec_private);

        void parse_block_group(const uint8_t* data, size_t size);

        void parse_block(const uint8_t* data, size_t size, std::optional<bool> keyframe);

        bytes::binary convert_payload(const uint8_t* data, size_t size, bool keyframe) const;

        static std::optional<std::pair<uint64_t, size_t>> read_vint(const uint8_t* data, size_t size, bool keep_marker);

        static uint64_t read_uint(const uint8_t* data, size_t size);

        static wrtc::models::Codec codec_from_id(const std::string& codec_id);

        static bool has_nal_type(const bytes::binary& annex_b, uint8_t type);

        static bool has_obu_type(const uint8_t* data, size_t size, uint8_t type);

    public:
        void feed(const uint8_t* data, size_t size);

        int64_t missing() const;

        std::optional<Frame> next();

        std::optional<Track> track() const;
    };

} // ntgcalls::media::containers
