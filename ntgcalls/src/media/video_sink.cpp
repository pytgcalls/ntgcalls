//
// Created by Lauren on 27/09/24.
//

#include <cmath>
#include <ntgcalls/media/video_sink.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::media {

    std::chrono::nanoseconds VideoSink::frame_time() {
        if (!description_) return std::chrono::nanoseconds(0);
        return std::chrono::microseconds(static_cast<uint64_t>(1000.0 * 1000.0 / static_cast<double_t>(description_->fps))); // ms
    }

    int64_t VideoSink::frame_size() {
        if (!description_) return 0;
        return std::llround(static_cast<float>(description_->width * description_->height) * 1.5f);
    }

    uint8_t VideoSink::frame_rate() {
        return description_ ? description_->fps : 0;
    }

    bool VideoSink::set_config(const std::optional<VideoDescription>& desc) {
        const bool changed = description_ != desc;
        const bool force_change = desc && !desc->keep_open;
        if (changed || force_change) {
            description_ = desc;
            clear();
            if (desc->width <= 0 && desc->height <= 0 && desc->fps == 0) {
                RTC_LOG(LS_INFO) << "VideoSink configured with auto resolution";
            } else {
                RTC_LOG(LS_INFO) << "VideoSink configured with " << desc->width << "x" << desc->height << "@" << desc->fps << "fps";
            }
        }
        return changed || force_change;
    }

    std::optional<VideoDescription> VideoSink::get_config() {
        return description_;
    }
} // ntgcalls::media
