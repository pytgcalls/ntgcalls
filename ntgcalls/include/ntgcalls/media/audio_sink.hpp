//
// Created by Laky64 on 27/09/24.
//

#pragma once

// PCM16L AUDIO CODEC SPECIFICATION
// Frame Time: 10ms
// Max SampleRate: 96000
// Max BitsPerSample: 16
// Max Channels: 2
// FrameSize: ((48000 * 16) / 8 / 100)) * 2

#include <ntgcalls/media/base_sink.hpp>
#include <ntgcalls/models/media_description.hpp>

namespace ntgcalls {

    class AudioSink: public BaseSink {
    protected:
        std::optional<AudioDescription> description;

    public:
        bool setConfig(const std::optional<AudioDescription>& desc);

        std::optional<AudioDescription> getConfig();

        std::chrono::nanoseconds frameTime() override;

        int64_t frameSize() override;
    };

} // ntgcalls
