//
// Created by Lauren on 27/09/24.
//

#include <ntgcalls/media/audio_sink.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::media {
    std::chrono::nanoseconds AudioSink::frame_time() {
        return 10ms;
    }

    int64_t AudioSink::frame_size() {
        if (!description_) return 0;
        return description_->sample_rate * 16 / 8 / 100 * description_->channel_count;
    }

    uint8_t AudioSink::frame_rate() {
        return 100;
    }

    bool AudioSink::set_config(const std::optional<AudioDescription>& desc) {
        const bool changed = description_ != desc;
        const bool force_change = desc && !desc->keep_open;
        if (changed || force_change) {
            description_ = desc;
            clear();
            RTC_LOG(LS_INFO) << "AudioSink configured with " << desc->sample_rate << "Hz, " << 16 << "bps, " << desc->channel_count << " channels";
        }
        return changed || force_change;
    }

    std::optional<AudioDescription> AudioSink::get_config() {
        return description_;
    }
} // ntgcalls::media
