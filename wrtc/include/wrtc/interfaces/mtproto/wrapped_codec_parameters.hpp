//
// Created by Lauren on 14/04/25.
//

#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace wrtc::interfaces::mtproto {

    class WrappedCodecParameters {
        AVCodecParameters* value_ = nullptr;

    public:
        explicit WrappedCodecParameters(const AVCodecParameters* codec_parameters);

        ~WrappedCodecParameters();

        bool is_equal(const AVCodecParameters* other) const;
    };

} // wrtc::interfaces::mtproto
