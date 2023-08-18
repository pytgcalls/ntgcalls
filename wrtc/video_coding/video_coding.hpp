//
// Created by Laky64 on 18/08/2023.
//

#pragma once

#include "types.hpp"
#include "software/vlc/vlc.hpp"
#include "software/google/google.hpp"

namespace wrtc {

    class VideoDecoder {
    public:


    private:
        VideoCoding CreateVideoCoding(const webrtc::SdpVideoFormat& format);

        std::shared_ptr<VideoFactoryConfig> GetVideoFactoryConfig();

        template <typename T>
        std::vector<std::vector<webrtc::SdpVideoFormat>> GetSupportedFormats(std::vector<T> configs) const;

        template <typename X, typename T>
        std::unique_ptr<X> CreateBaseVideo(std::vector<T> input, const webrtc::SdpVideoFormat& format);
    };

} // wrtc
