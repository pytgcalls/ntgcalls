//
// Created by Laky64 on 02/10/24.
//

#include <wrtc/models/simulcast_layer.hpp>

namespace wrtc {
    SimulcastLayer::SimulcastLayer(const uint32_t ssrc, const uint32_t fidSsrc) : ssrc(ssrc), fidSsrc(fidSsrc) {}
} // wrtc