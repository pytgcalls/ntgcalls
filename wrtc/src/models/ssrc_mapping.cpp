//
// Created by Lauren on 07/06/26.
//

#include <wrtc/models/ssrc_mapping.hpp>

namespace wrtc::models {

    SsrcMapping::SsrcMapping(const int64_t user_id, const int32_t ssrc): user_id(user_id), ssrc(ssrc) {}

} // wrtc::models