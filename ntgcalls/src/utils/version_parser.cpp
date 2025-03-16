//
// Created by Laky64 on 13/10/24.
//

#include <ntgcalls/utils/version_parser.hpp>
#include <sstream>
#include <vector>

namespace ntgcalls {
    VersionParser VersionParser::Parse(const std::string& version) {
        try {
            std::vector<std::string> parts;
            std::istringstream stream(version);
            std::string part;
            while (std::getline(stream, part, '.')) {
                parts.push_back(part);
            }
            if (parts.size() != 3) {
                return {};
            }
            return {std::stoi(parts[0]), std::stoi(parts[1]), std::stoi(parts[2])};
        } catch (const std::invalid_argument&) {
            return {};
        }
    }

    bool VersionParser::operator>=(const VersionParser& other) {
        return std::tie(major, minor, micro) >= std::tie(other.major, other.minor, other.micro);
    }

    bool VersionParser::operator<=(const VersionParser& other) {
        return std::tie(major, minor, micro) <= std::tie(other.major, other.minor, other.micro);
    }

    bool VersionParser::operator<(const VersionParser& other) {
        return std::tie(major, minor, micro) < std::tie(other.major, other.minor, other.micro);
    }

    bool VersionParser::operator>(const VersionParser& other) {
        return std::tie(major, minor, micro) > std::tie(other.major, other.minor, other.micro);
    }

    bool VersionParser::operator==(const VersionParser& other) {
        return std::tie(major, minor, micro) == std::tie(other.major, other.minor, other.micro);
    }

    std::string VersionParser::toString() const {
        return std::to_string(major) + '.' + std::to_string(minor) + '.' + std::to_string(micro);
    }
} // ntgcalls