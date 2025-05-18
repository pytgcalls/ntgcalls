//
// Created by Laky64 on 05/09/2024.
//

#include <utility>
#include <wrtc/interfaces/media/tracks/media_track_interface.hpp>

namespace wrtc {
    MediaTrackInterface::MediaTrackInterface(const std::function<void(bool)>& enableCallback) {
        this->enableCallback = enableCallback;
    }

    bool MediaTrackInterface::set_enabled(const bool enable) {
        (void) enableCallback(enable);
        return std::exchange(status, enable) != enable;
    }

    bool MediaTrackInterface::enabled() const {
        return status;
    }
} // wrtc