//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <optional>
#include <string>
#include <ntgcalls/media/media_description.hpp>

namespace ntgcalls::media::containers {

    class LibAvFormat {
    public:
        static const char* input_name(BaseMediaDescription::Format format);

        static const char* output_name(BaseMediaDescription::Format format);

        static std::optional<BaseMediaDescription::Format> from_path(const std::string& path);
    };

} // ntgcalls::media::containers
