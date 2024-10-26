//
// Created by Laky64 on 22/03/2024.
//

#pragma once
#include <nlohmann/json.hpp>
#include <rtc_base/byte_buffer.h>
#include <rtc_base/copy_on_write_buffer.h>

#include <wrtc/utils/binary.hpp>

namespace signaling {
    using nlohmann::json;

    class Message {
    public:
        enum class Type {
            Candidate,
            RtcDescription,
            InitialSetup,
            Candidates,
            NegotiateChannels,
            MediaState,
            Unknown
        };

        virtual ~Message() = default;

        [[nodiscard]] virtual bytes::binary serialize() const = 0;

        static Type type(const bytes::binary& data);

        static std::optional<rtc::CopyOnWriteBuffer> deserializeRaw(rtc::ByteBufferReader &reader);

        static uint32_t stringToUInt32(std::string const &string);
    };

} // wrtc
