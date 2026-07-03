//
// Created by Lauren on 05/09/24.
//

#pragma once

#include <wrtc/utils/synchronized_callback.hpp>

namespace wrtc::interfaces::media::tracks {
    class MediaTrackInterface {
        utils::synchronized_callback<void(bool)> enable_callback_;
        bool status_ = true;

    public:
        explicit MediaTrackInterface(const std::function<void(bool)>& enable_callback);

        ~MediaTrackInterface();

        bool set_enabled(bool enable);

        bool enabled() const;
    };
} // wrtc::interfaces::media::tracks
