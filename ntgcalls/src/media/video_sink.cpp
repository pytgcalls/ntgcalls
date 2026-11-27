//
// Created by Lauren on 27/09/24.
//

#include <cmath>
#include <ntgcalls/media/video_sink.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::media {

    std::chrono::nanoseconds VideoSink::frame_time() {
        const auto description = this->description();
        if (!description) return std::chrono::nanoseconds(0);
        return std::chrono::microseconds(static_cast<uint64_t>(1000.0 * 1000.0 / static_cast<double_t>(description->fps))); // ms
    }

    int64_t VideoSink::frame_size() {
        const auto description = this->description();
        if (!description) return 0;
        return std::llround(static_cast<float>(description->width * description->height) * 1.5f);
    }

    uint8_t VideoSink::frame_rate() {
        const auto description = this->description();
        return description ? description->fps : 0;
    }

    bool VideoSink::set_config(const std::optional<VideoDescription>& desc) {
        const std::lock_guard lock(description_mutex_);
        const bool changed = desc ? !description_ || *description_ != *desc : static_cast<bool>(description_);
        const bool force_change = desc && !desc->keep_open;
        if (changed || force_change) {
            description_ = desc ? std::make_shared<const VideoDescription>(*desc) : nullptr;
            source_frame_rate_ = 0;
            source_reordered_ = false;
            clear();
            if (!desc) {
                RTC_LOG(LS_INFO) << "VideoSink configuration cleared";
            } else if (desc->width <= 0 && desc->height <= 0 && desc->fps == 0) {
                RTC_LOG(LS_INFO) << "VideoSink configured with auto resolution";
            } else {
                RTC_LOG(LS_INFO) << "VideoSink configured with " << desc->width << "x" << desc->height << "@" << desc->fps << "fps";
            }
        }
        return changed || force_change;
    }

    std::optional<VideoDescription> VideoSink::get_config() const {
        if (const auto description = this->description()) {
            return *description;
        }
        return std::nullopt;
    }

    std::shared_ptr<const VideoDescription> VideoSink::description() const {
        const std::lock_guard lock(description_mutex_);
        return description_;
    }

    void VideoSink::set_source_frame_rate(const double fps) {
        source_frame_rate_ = fps;
    }

    double VideoSink::source_frame_rate() const {
        return source_frame_rate_;
    }

    void VideoSink::set_source_reordered(const bool reordered) {
        source_reordered_ = reordered;
    }

    bool VideoSink::source_reordered() const {
        return source_reordered_;
    }
} // ntgcalls::media
