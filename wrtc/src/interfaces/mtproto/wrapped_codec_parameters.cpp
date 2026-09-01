//
// Created by Lauren on 14/04/25.
//

#include <wrtc/interfaces/mtproto/wrapped_codec_parameters.hpp>

namespace wrtc::interfaces::mtproto {
    WrappedCodecParameters::WrappedCodecParameters(const AVCodecParameters* codec_parameters) {
        value_ = avcodec_parameters_alloc();
        avcodec_parameters_copy(value_, codec_parameters);
    }

    WrappedCodecParameters::~WrappedCodecParameters() {
        avcodec_parameters_free(&value_);
    }

    bool WrappedCodecParameters::is_equal(const AVCodecParameters* other) const {
        return value_->codec_id == other->codec_id &&
               value_->format == other->format &&
               value_->ch_layout.nb_channels == other->ch_layout.nb_channels;
    }
} // wrtc::interfaces::mtproto
