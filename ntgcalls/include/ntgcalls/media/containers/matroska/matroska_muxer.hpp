//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <optional>
#include <string>
#include <vector>
#include <ntgcalls/media/containers/media_kind.hpp>
#include <ntgcalls/media/containers/muxer.hpp>
#include <ntgcalls/media/containers/track_info.hpp>

namespace ntgcalls::media::containers {

    class MatroskaMuxer final: public Muxer {
        static constexpr int64_t kMaxClusterDuration = 30000;

        wrtc::models::Codec codec_;
        uint16_t width_;
        uint16_t height_;
        bool header_written_ = false;
        std::optional<int64_t> first_timestamp_;
        std::optional<int64_t> cluster_timestamp_;

        bytes::binary header(const bytes::binary& codec_private) const;

        bytes::binary cluster(int64_t timestamp) const;

        bytes::binary block(const bytes::binary& payload, int64_t relative, bool keyframe) const;

        bytes::binary payload(const uint8_t* data, size_t size) const;

        static std::vector<std::pair<const uint8_t*, size_t>> split_annex_b(const uint8_t* data, size_t size);

        static bytes::binary avc_configuration(const uint8_t* data, size_t size);

        static std::string codec_id(wrtc::models::Codec codec);

        static bytes::binary element(uint32_t id, const bytes::binary& payload);

        static bytes::binary uint_element(uint32_t id, uint64_t value);

        static bytes::binary string_element(uint32_t id, const std::string& value);

        static void append_id(bytes::binary& output, uint32_t id);

        static void append_size(bytes::binary& output, uint64_t size);

    public:
        explicit MatroskaMuxer(const TrackInfo& track);

        MuxedChunk add(EncodedPacket packet) override;

        MuxedChunk finish() override;

        static constexpr bool supports(const MediaKind kind) {
            return kind == MediaKind::Video;
        }

        static bool supports(wrtc::models::Codec codec);
    };

} // ntgcalls::media::containers
