//
// Created by Lauren on 27/11/26.
//

#include <algorithm>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/codecs/pcm_resampler.hpp>

namespace ntgcalls::media::codecs {
    PcmResampler::PcmResampler(const AVChannelLayout& input_layout, const AVSampleFormat input_format, const int input_rate, const uint8_t channels, const int output_rate): channels_(channels) {
        AVChannelLayout input;
        if (input_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
            av_channel_layout_default(&input, input_layout.nb_channels);
        } else {
            av_channel_layout_copy(&input, &input_layout);
        }
        AVChannelLayout output;
        av_channel_layout_default(&output, channels);
        const int error = swr_alloc_set_opts2(&context_, &output, AV_SAMPLE_FMT_S16, output_rate, &input, input_format, input_rate, 0, nullptr);
        av_channel_layout_uninit(&input);
        av_channel_layout_uninit(&output);
        if (error < 0 || swr_init(context_) < 0) {
            swr_free(&context_);
            throw FFmpegError("Unable to configure the audio resampler");
        }
    }

    PcmResampler::~PcmResampler() {
        swr_free(&context_);
    }

    void PcmResampler::convert(const uint8_t* const* input, const int samples, std::vector<int16_t>& output) const {
        const int capacity = swr_get_out_samples(context_, samples);
        if (capacity <= 0) {
            return;
        }
        const auto offset = output.size();
        output.resize(offset + static_cast<size_t>(capacity) * channels_);
        uint8_t* const planes[] = {reinterpret_cast<uint8_t*>(output.data() + offset)};
        const int converted = swr_convert(context_, planes, capacity, input, samples);
        output.resize(offset + static_cast<size_t>(std::max(converted, 0)) * channels_);
    }

    void PcmResampler::flush(std::vector<int16_t>& output) const {
        convert(nullptr, 0, output);
    }
} // ntgcalls::media::codecs
