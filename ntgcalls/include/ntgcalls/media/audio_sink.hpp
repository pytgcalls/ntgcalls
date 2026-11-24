//
// Created by Lauren on 27/09/24.
//

#pragma once

// PCM16L AUDIO CODEC SPECIFICATION
// Frame Time: 10 ms
// Max SampleRate: 96000
// Max BitsPerSample: 16
// Max Channels: 2
// FrameSize: ((48000 * 16) / 8 / 100)) * 2

#include <memory>
#include <mutex>
#include <ntgcalls/media/base_sink.hpp>
#include <ntgcalls/media/media_description.hpp>

namespace ntgcalls::media {
    using namespace std::chrono_literals;

    class AudioSink: public BaseSink {
        std::shared_ptr<const AudioDescription> description_;
        mutable std::mutex description_mutex_;

    protected:
        std::shared_ptr<const AudioDescription> description() const;

    public:
        bool set_config(const std::optional<AudioDescription>& desc);

        std::optional<AudioDescription> get_config();

        std::chrono::nanoseconds frame_time() override;

        int64_t frame_size() override;

        static int64_t frame_size_of(const AudioDescription& description);

        uint8_t frame_rate() override;
    };

} // ntgcalls::media
