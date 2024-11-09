//
// Created by Laky64 on 27/09/24.
//

#include <ntgcalls/media/audio_sink.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls {
    std::chrono::nanoseconds AudioSink::frameTime() {
        return std::chrono::milliseconds(10); // ms
    }

    int64_t AudioSink::frameSize() {
        if (!description) return 0;
        return description->sampleRate * 16 / 8 / 100 * description->channelCount;
    }

    bool AudioSink::setConfig(const std::optional<AudioDescription>& desc) {
        const bool changed = description != desc;
        if (changed) {
            description = desc;
            clear();
            RTC_LOG(LS_INFO) << "AudioSink configured with " << desc->sampleRate << "Hz, " << 16 << "bps, " << desc->channelCount << " channels";
        }
        return changed;
    }

    std::optional<AudioDescription> AudioSink::getConfig() {
        return description;
    }
} // ntgcalls