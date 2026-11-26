//
// Created by Lauren on 27/11/26.
//

#pragma once

#include <vector>

extern "C" {
#include <libswresample/swresample.h>
}

namespace ntgcalls::media::codecs {

    class PcmResampler {
        SwrContext* context_ = nullptr;
        uint8_t channels_;

    public:
        PcmResampler(const AVChannelLayout& input_layout, AVSampleFormat input_format, int input_rate, uint8_t channels, int output_rate);

        ~PcmResampler();

        PcmResampler(const PcmResampler&) = delete;

        PcmResampler& operator=(const PcmResampler&) = delete;

        void convert(const uint8_t* const* input, int samples, std::vector<int16_t>& output) const;

        void flush(std::vector<int16_t>& output) const;
    };

} // ntgcalls::media::codecs
