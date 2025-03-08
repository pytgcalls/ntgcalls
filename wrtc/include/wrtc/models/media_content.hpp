//
// Created by Laky64 on 30/03/2024.
//

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <api/rtp_parameters.h>

namespace wrtc {
    struct SsrcGroup {
        std::string semantics;
        std::vector<uint32_t> ssrcs;

        bool operator==(SsrcGroup const &rhs) const {
            if (ssrcs != rhs.ssrcs) {
                return false;
            }
            if (semantics != rhs.semantics) {
                return false;
            }
            return true;
        }
    };

    struct FeedbackType {
        std::string type;
        std::string subtype;

        bool operator==(FeedbackType const &rhs) const {
            if (type != rhs.type) {
                return false;
            }
            if (subtype != rhs.subtype) {
                return false;
            }

            return true;
        }
    };

    struct PayloadType {
        uint32_t id = 0;
        std::string name;
        uint32_t clockrate = 0;
        uint32_t channels = 0;
        std::vector<FeedbackType> feedbackTypes;
        std::vector<std::pair<std::string, std::string>> parameters;

        bool operator==(PayloadType const &rhs) const {
            if (id != rhs.id) {
                return false;
            }
            if (name != rhs.name) {
                return false;
            }
            if (clockrate != rhs.clockrate) {
                return false;
            }
            if (channels != rhs.channels) {
                return false;
            }
            if (feedbackTypes != rhs.feedbackTypes) {
                return false;
            }
            if (parameters != rhs.parameters) {
                return false;
            }

            return true;
        }
    };

    struct MediaContent {
        enum class Type {
            Audio = 1 << 0,
            Video = 1 << 1
        };

        Type type = Type::Audio;
        uint32_t ssrc = 0;
        std::vector<SsrcGroup> ssrcGroups;
        std::vector<PayloadType> payloadTypes;
        std::vector<webrtc::RtpExtension> rtpExtensions;

        bool operator==(const MediaContent& rhs) const {
            if (type != rhs.type) {
                return false;
            }
            if (ssrc != rhs.ssrc) {
                return false;
            }
            if (ssrcGroups != rhs.ssrcGroups) {
                return false;
            }

            std::vector<PayloadType> sortedPayloadTypes = payloadTypes;
            std::ranges::sort(sortedPayloadTypes, [](PayloadType const &lhs, PayloadType const &rhs2) {
                return lhs.id < rhs2.id;
            });
            std::vector<PayloadType> sortedRhsPayloadTypes = rhs.payloadTypes;
            std::ranges::sort(sortedRhsPayloadTypes, [](PayloadType const &lhs, PayloadType const &rhs2) {
                return lhs.id < rhs2.id;
            });
            if (sortedPayloadTypes != sortedRhsPayloadTypes) {
                return false;
            }

            if (rtpExtensions != rhs.rtpExtensions) {
                return false;
            }

            return true;
        }

        [[nodiscard]] bool isScreenCast() const {
            return std::ranges::any_of(ssrcGroups, [](const auto& group) {
                return group.semantics == "SIM" && group.ssrcs.size() == 2;
            });
        }

        [[nodiscard]] uint32_t mainSsrc() const {
            if (ssrcGroups.size() <= 1) {
                return ssrc;
            }
            for (const auto& [semantics, ssrcs] : ssrcGroups) {
                if (semantics == "SIM") {
                    return ssrcs[0];
                }
            }
            return 0;
        }
    };
} // wrtc