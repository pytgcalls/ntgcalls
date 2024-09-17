//
// Created by Laky64 on 05/09/2024.
//

#pragma once

#include <wrtc/utils/syncronized_callback.hpp>
#include <atomic>

namespace wrtc {
    class MediaTrackInterface {
        synchronized_callback<bool> enableCallback;
        std::atomic_bool status = true;

    public:
        explicit MediaTrackInterface(const std::function<void(bool)>& enableCallback);

        void set_enabled(bool status);

        bool enabled() const;
    };
} // wrtc
