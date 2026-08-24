//
// Created by Lauren on 22/03/24.
//

#pragma once
#include <rtc_base/byte_buffer.h>
#include <rtc_base/copy_on_write_buffer.h>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/json.hpp>

namespace ntgcalls::signaling::messages {
    using wrtc::utils::json;

    class Message {
    public:
        enum class Type {
            InitialSetup,
            Candidates,
            NegotiateChannels,
            MediaState,
            Unknown
        };

        virtual ~Message() = default;

        [[nodiscard]] virtual bytes::binary serialize() const = 0;

        static Type type(const bytes::binary& data);

        static std::optional<webrtc::CopyOnWriteBuffer> deserialize_raw(webrtc::ByteBufferReader& reader);

        static uint32_t string_to_uint32(std::string const& string);
    };

} // ntgcalls::signaling::messages
