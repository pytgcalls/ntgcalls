//
// Created by Lauren on 05/09/24.
//

#include <utility>
#include <wrtc/interfaces/media/tracks/media_track_interface.hpp>

namespace wrtc::interfaces::media::tracks {
    MediaTrackInterface::MediaTrackInterface(const std::function<void(bool)>& enable_callback) {
        this->enable_callback_ = enable_callback;
    }

    MediaTrackInterface::~MediaTrackInterface() {
        enable_callback_ = nullptr;
    }

    bool MediaTrackInterface::set_enabled(const bool enable) {
        (void) enable_callback_(enable);
        return std::exchange(status_, enable) != enable;
    }

    bool MediaTrackInterface::enabled() const {
        return status_;
    }
} // wrtc::interfaces::media::tracks