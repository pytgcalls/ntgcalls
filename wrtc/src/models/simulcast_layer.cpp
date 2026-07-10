//
// Created by Lauren on 02/10/24.
//

#include <wrtc/models/simulcast_layer.hpp>

namespace wrtc::models {
    SimulcastLayer::SimulcastLayer(const uint32_t ssrc, const uint32_t fid_ssrc) : ssrc(ssrc), fid_ssrc(fid_ssrc) {}
} // wrtc::models