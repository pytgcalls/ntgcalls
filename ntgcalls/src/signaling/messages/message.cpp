//
// Created by Laky64 on 22/03/2024.
//
#include <ntgcalls/signaling/messages/message.hpp>

#include <ntgcalls/exceptions.hpp>

namespace signaling {
    Message::Type Message::type(const bytes::binary& data) {
        if (data.empty()) {
            throw ntgcalls::InvalidParams("Empty data");
        }
        auto j = json::parse(data.begin(), data.end());
        if (const auto type = j["@type"];!type.is_null()) {
            if (type == "candidate") {
                return Type::Candidate;
            }
            if (type == "offer" || type == "answer") {
                return Type::RtcDescription;
            }
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

    std::optional<rtc::CopyOnWriteBuffer> Message::deserializeRaw(rtc::ByteBufferReader &reader) {
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
        rtc::CopyOnWriteBuffer result;
        result.SetSize(length);
        if (!reader.ReadBytes(rtc::MakeArrayView(result.MutableData(), result.size()))) {
            return std::nullopt;
        }
        return result;
    }

    uint32_t Message::stringToUInt32(std::string const &string) {
        std::stringstream stringStream(string);
        uint32_t value = 0;
        stringStream >> value;
        return value;
    }
} // signaling