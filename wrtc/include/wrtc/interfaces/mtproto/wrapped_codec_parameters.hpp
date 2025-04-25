//
// Created by Laky64 on 14/04/25.
//

#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace wrtc {

    class WrappedCodecParameters {
        AVCodecParameters *value = nullptr;

    public:
        explicit WrappedCodecParameters(const AVCodecParameters *codecParameters);

        ~WrappedCodecParameters();

        bool isEqual(const AVCodecParameters *other) const;
    };

} // wrtc
