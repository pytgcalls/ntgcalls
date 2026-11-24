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
        const auto description = this->description();
        if (!description) return 0;
        return frame_size_of(*description);
    }

    int64_t AudioSink::frame_size_of(const AudioDescription& description) {
        return description.sample_rate * 16 / 8 / 100 * description.channel_count;
    }

    uint8_t AudioSink::frame_rate() {
        return 100;
    }

    bool AudioSink::set_config(const std::optional<AudioDescription>& desc) {
        const std::lock_guard lock(description_mutex_);
        const bool changed = desc ? !description_ || *description_ != *desc : static_cast<bool>(description_);
        const bool force_change = desc && !desc->keep_open;
        if (changed || force_change) {
            description_ = desc ? std::make_shared<const AudioDescription>(*desc) : nullptr;
            clear();
            RTC_LOG(LS_INFO) << "AudioSink configured with " << desc->sample_rate << "Hz, " << 16 << "bps, " << desc->channel_count << " channels";
        }
        return changed || force_change;
    }

    std::optional<AudioDescription> AudioSink::get_config() {
        if (const auto description = this->description()) {
            return *description;
        }
        return std::nullopt;
    }

    std::shared_ptr<const AudioDescription> AudioSink::description() const {
        const std::lock_guard lock(description_mutex_);
        return description_;
    }
} // ntgcalls::media
