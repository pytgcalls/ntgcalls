//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include "video_encoder_config.hpp"
#include "video_decoder_config.hpp"
#include "software/vlc/vlc.hpp"
#include "software/google/google.hpp"
#include "video_factory_config.hpp"

namespace wrtc {

    class BaseVideoFactory {
    private:
        mutable std::vector<std::vector<webrtc::SdpVideoFormat>> formats_;

    protected:
        template <typename T>
        std::vector<webrtc::SdpVideoFormat> internalFormats(std::vector<T> configs) const;

        template <typename X, typename T>
        std::unique_ptr<X> internalVideo(std::vector<T> input, const webrtc::SdpVideoFormat& format);
    };

} // wrtc
