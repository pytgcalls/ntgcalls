//
// Created by laky64 on 07/06/26.
//

#include <wrtc/models/ssrc_mapping.hpp>

namespace wrtc {

    SsrcMapping::SsrcMapping(const int64_t userID, const int32_t ssrc): userID(userID), ssrc(ssrc) {}

} // wrtc