//
// Created by Laky64 on 13/10/24.
//

#pragma once

#include <string>

namespace ntgcalls {

    struct VersionParser {
        int major = 0;
        int minor = 0;
        int micro = 0;

        static VersionParser Parse(const std::string& version);

        bool operator>=(const VersionParser& other);

        bool operator<=(const VersionParser& other);

        bool operator<(const VersionParser& other);

        bool operator>(const VersionParser& other);

        bool operator==(const VersionParser& other);

        [[nodiscard]] std::string toString() const;
    };

} // ntgcalls
