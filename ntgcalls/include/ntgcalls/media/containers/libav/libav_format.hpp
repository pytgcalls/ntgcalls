//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <optional>
#include <string>
#include <ntgcalls/media/containers/media_kind.hpp>
#include <ntgcalls/media/media_description.hpp>

extern "C" {
#include <libavcodec/codec_id.h>
}

namespace ntgcalls::media::containers {

    class LibAvFormat {
    public:
        static const char* input_name(BaseMediaDescription::Format format);

        static const char* output_name(BaseMediaDescription::Format format);

        static std::optional<BaseMediaDescription::Format> from_path(const std::string& path);

        static constexpr bool holds(const BaseMediaDescription::Format format, const MediaKind kind) {
            switch (format) {
            case BaseMediaDescription::Format::Matroska:
            case BaseMediaDescription::Format::Webm:
            case BaseMediaDescription::Format::MP4:
                return true;
            default:
                return kind == MediaKind::Audio;
            }
        }

        static constexpr AVCodecID pcm_encoder(const BaseMediaDescription::Format format) {
            switch (format) {
            case BaseMediaDescription::Format::MP4:
                return AV_CODEC_ID_AAC;
            case BaseMediaDescription::Format::FLAC:
                return AV_CODEC_ID_FLAC;
            default:
                return AV_CODEC_ID_NONE;
            }
        }
    };

} // ntgcalls::media::containers
