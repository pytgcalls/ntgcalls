//
// Created by Laky64 on 22/03/2024.
//
#include "message.hpp"

#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {
    Message::Type Message::type(const bytes::binary& data) {
        if (data.empty()) {
            throw InvalidParams("Empty data");
        }
        auto j = json::parse(data.begin(), data.end());
        if (const auto type = j["@type"];!type.is_null()) {
            if (type == "candidate") {
                return Type::Candidate;
            }
            if (type == "offer" || type == "answer") {
                return Type::RtcDescription;
            }
        }
        return Type::Unknown;
    }
} // ntgcalls