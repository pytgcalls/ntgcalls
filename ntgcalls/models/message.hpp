//
// Created by Laky64 on 22/03/2024.
//

#pragma once
#include <nlohmann/json.hpp>
#include "wrtc/utils/binary.hpp"

namespace ntgcalls {
    using nlohmann::json;

    class Message {
    public:
        enum class Type {
            Candidate,
            RtcDescription,
            Unknown
        };

        virtual ~Message() = default;

        [[nodiscard]] virtual bytes::binary serialize() const = 0;

        static Type type(const bytes::binary& data);
    };

} // wrtc
