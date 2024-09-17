//
// Created by Laky64 on 05/09/2024.
//

#include <wrtc/interfaces/media/tracks/media_track_interface.hpp>

namespace wrtc {
    MediaTrackInterface::MediaTrackInterface(const std::function<void(bool)>& enableCallback) {
        this->enableCallback = enableCallback;
    }

    void MediaTrackInterface::set_enabled(const bool status) {
        (void) enableCallback(status);
        this->status = status;
    }

    bool MediaTrackInterface::enabled() const {
        return this->status;
    }
} // wrtc