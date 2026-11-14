//
// Created by Lauren on 05/09/24.
//

#pragma once

#include <wrtc/models/audio_encoding.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace wrtc::interfaces::media::tracks {
    class MediaTrackInterface {
        utils::synchronized_callback<void(bool)> enable_callback_;
        utils::synchronized_callback<void(models::AudioEncoding)> encoding_callback_;
        bool status_ = true;

    public:
        explicit MediaTrackInterface(const std::function<void(bool)>& enable_callback, const std::function<void(models::AudioEncoding)>& encoding_callback = nullptr);

        ~MediaTrackInterface();

        bool set_enabled(bool enable);

        bool enabled() const;

        void set_encoding(const models::AudioEncoding& encoding) const;
    };
} // wrtc::interfaces::media::tracks
