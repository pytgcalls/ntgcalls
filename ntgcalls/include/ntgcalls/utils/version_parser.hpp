//
// Created by Lauren on 13/10/24.
//

#pragma once

#include <string>

namespace ntgcalls::utils {

    struct VersionParser {
        int major = 0;
        int minor = 0;
        int micro = 0;

        static VersionParser parse(const std::string& version);

        bool operator>=(const VersionParser& other);

        bool operator<=(const VersionParser& other);

        bool operator<(const VersionParser& other);

        bool operator>(const VersionParser& other);

        bool operator==(const VersionParser& other);

        [[nodiscard]] std::string to_string() const;
    };

} // ntgcalls::utils
