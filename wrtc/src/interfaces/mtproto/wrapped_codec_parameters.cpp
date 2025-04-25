//
// Created by Laky64 on 14/04/25.
//

#include <rtc_base/logging.h>
#include <wrtc/interfaces/mtproto/wrapped_codec_parameters.hpp>

namespace wrtc {
    WrappedCodecParameters::WrappedCodecParameters(const AVCodecParameters* codecParameters) {
        value = avcodec_parameters_alloc();
        avcodec_parameters_copy(value, codecParameters);
    }

    WrappedCodecParameters::~WrappedCodecParameters() {
        avcodec_parameters_free(&value);
    }

    bool WrappedCodecParameters::isEqual(const AVCodecParameters* other) const {
        return value->codec_id == other->codec_id &&
               value->format == other->format &&
                   value->ch_layout.nb_channels == other->ch_layout.nb_channels;
    }
} // wrtc