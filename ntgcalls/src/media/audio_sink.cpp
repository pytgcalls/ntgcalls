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
        return description->sampleRate * description->bitsPerSample / 8 / 100 * description->channelCount;
    }

    bool AudioSink::setConfig(const std::optional<AudioDescription>& desc) {
        clear();
        const bool changed = description != desc;
        description = desc;
        if (changed) {
            RTC_LOG(LS_INFO) << "AudioSink configured with " << desc->sampleRate << "Hz, " << desc->bitsPerSample << "bps, " << desc->channelCount << " channels";
        }
        return changed;
    }
} // ntgcalls