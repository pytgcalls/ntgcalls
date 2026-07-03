//
// Created by Lauren on 22/03/24.
//
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/signaling/messages/message.hpp>

namespace ntgcalls::signaling::messages {
    Message::Type Message::type(const bytes::binary& data) {
        if (data.empty()) {
            throw InvalidParams("Empty data");
        }
        auto j = json::parse(data.begin(), data.end());
        if (const auto type = j["@type"];!type.is_null()) {
            if (type == "InitialSetup") {
                return Type::InitialSetup;
            }
            if (type == "Candidates") {
                return Type::Candidates;
            }
            if (type == "NegotiateChannels") {
                return Type::NegotiateChannels;
            }
            if (type == "MediaState") {
                return Type::MediaState;
            }
        }
        return Type::Unknown;
    }

    std::optional<webrtc::CopyOnWriteBuffer> Message::deserialize_raw(webrtc::ByteBufferReader &reader) {
        if (!reader.Length()) {
            return std::nullopt;
        }
        uint32_t length = 0;
        if (!reader.ReadUInt32(&length)) {
            return std::nullopt;
        }
        if (length > 1024 * 1024) {
            return std::nullopt;
        }
        webrtc::CopyOnWriteBuffer result;
        result.SetSize(length);
        if (!reader.ReadBytes(std::span(result.MutableData(), result.size()))) {
            return std::nullopt;
        }
        return result;
    }

    uint32_t Message::string_to_uint32(std::string const &string) {
        std::stringstream string_stream(string);
        uint32_t value = 0;
        string_stream >> value;
        return value;
    }
} // ntgcalls::signaling::messages