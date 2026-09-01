//
// Created by Lauren on 30/03/24.
//

#pragma once
#include <string>
#include <vector>
#include <api/rtp_parameters.h>

namespace wrtc::models {
    struct SsrcGroup {
        std::string semantics;
        std::vector<uint32_t> ssrcs;

        bool operator==(SsrcGroup const& rhs) const {
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

        bool operator==(FeedbackType const& rhs) const {
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
        int32_t id = 0;
        std::string name;
        int32_t clockrate = 0;
        size_t channels = 0;
        std::vector<FeedbackType> feedback_types;
        std::vector<std::pair<std::string, std::string>> parameters;

        bool operator==(PayloadType const& rhs) const {
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
            if (feedback_types != rhs.feedback_types) {
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
        int64_t user_id = 0;
        std::vector<SsrcGroup> ssrc_groups;
        std::vector<PayloadType> payload_types;
        std::vector<webrtc::RtpExtension> rtp_extensions;

        bool operator==(const MediaContent& rhs) const {
            if (type != rhs.type) {
                return false;
            }
            if (ssrc != rhs.ssrc) {
                return false;
            }
            if (ssrc_groups != rhs.ssrc_groups) {
                return false;
            }

            std::vector<PayloadType> sorted_payload_types = payload_types;
            std::ranges::sort(sorted_payload_types, [](PayloadType const& lhs, PayloadType const& rhs2) {
                return lhs.id < rhs2.id;
            });
            std::vector<PayloadType> sorted_rhs_payload_types = rhs.payload_types;
            std::ranges::sort(sorted_rhs_payload_types, [](PayloadType const& lhs, PayloadType const& rhs2) {
                return lhs.id < rhs2.id;
            });
            if (sorted_payload_types != sorted_rhs_payload_types) {
                return false;
            }

            if (rtp_extensions != rhs.rtp_extensions) {
                return false;
            }

            return true;
        }

        [[nodiscard]] bool is_screen_cast() const {
            return std::ranges::any_of(ssrc_groups, [](const auto& group) {
                return group.semantics == "SIM" && group.ssrcs.size() == 2;
            });
        }

        [[nodiscard]] uint32_t main_ssrc() const {
            if (ssrc_groups.size() <= 1) {
                return ssrc;
            }
            for (const auto& [semantics, ssrcs] : ssrc_groups) {
                if (semantics == "SIM") {
                    return ssrcs[0];
                }
            }
            return 0;
        }
    };
} // wrtc::models
