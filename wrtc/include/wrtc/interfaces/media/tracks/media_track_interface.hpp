//
// Created by Laky64 on 05/09/2024.
//

#pragma once

#include <wrtc/utils/synchronized_callback.hpp>

namespace wrtc {
    class MediaTrackInterface {
        synchronized_callback<bool> enableCallback;
        bool status = true;

    public:
        explicit MediaTrackInterface(const std::function<void(bool)>& enableCallback);

        bool set_enabled(bool enable);

        bool enabled() const;
    };
} // wrtc
