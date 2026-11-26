//
// Created by Lauren on 27/11/26.
//

#include <array>
#include <cctype>
#include <ntgcalls/media/containers/libav/libav_format.hpp>

namespace ntgcalls::media::containers {
    const char* LibAvFormat::input_name(const BaseMediaDescription::Format format) {
        switch (format) {
        case BaseMediaDescription::Format::Ogg:
            return "ogg";
        case BaseMediaDescription::Format::Matroska:
        case BaseMediaDescription::Format::Webm:
            return "matroska";
        case BaseMediaDescription::Format::MP4:
            return "mov";
        case BaseMediaDescription::Format::WAV:
            return "wav";
        case BaseMediaDescription::Format::FLAC:
            return "flac";
        default:
            return nullptr;
        }
    }

    const char* LibAvFormat::output_name(const BaseMediaDescription::Format format) {
        switch (format) {
        case BaseMediaDescription::Format::Ogg:
            return "ogg";
        case BaseMediaDescription::Format::Matroska:
            return "matroska";
        case BaseMediaDescription::Format::Webm:
            return "webm";
        case BaseMediaDescription::Format::MP4:
            return "mp4";
        case BaseMediaDescription::Format::WAV:
            return "wav";
        case BaseMediaDescription::Format::FLAC:
            return "flac";
        default:
            return nullptr;
        }
    }

    std::optional<BaseMediaDescription::Format> LibAvFormat::from_path(const std::string& path) {
        constexpr std::array<std::pair<std::string_view, BaseMediaDescription::Format>, 11> kExtensions = {{
            {"ogg", BaseMediaDescription::Format::Ogg},
            {"opus", BaseMediaDescription::Format::Ogg},
            {"oga", BaseMediaDescription::Format::Ogg},
            {"mkv", BaseMediaDescription::Format::Matroska},
            {"mka", BaseMediaDescription::Format::Matroska},
            {"webm", BaseMediaDescription::Format::Webm},
            {"mp4", BaseMediaDescription::Format::MP4},
            {"m4a", BaseMediaDescription::Format::MP4},
            {"mov", BaseMediaDescription::Format::MP4},
            {"wav", BaseMediaDescription::Format::WAV},
            {"flac", BaseMediaDescription::Format::FLAC},
        }};
        const auto dot = path.find_last_of('.');
        if (dot == std::string::npos || path.find_first_of("/\\", dot) != std::string::npos) {
            return std::nullopt;
        }
        std::string extension = path.substr(dot + 1);
        for (auto& c : extension) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        for (const auto& [name, format] : kExtensions) {
            if (extension == name) {
                return format;
            }
        }
        return std::nullopt;
    }
} // ntgcalls::media::containers
