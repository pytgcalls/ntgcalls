//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <memory>
#include <ntgcalls/io/threaded_writer.hpp>
#include <ntgcalls/media/composer/video_composer.hpp>

namespace ntgcalls::io {

    class RawWriter: public ThreadedWriter {
        std::unique_ptr<media::composer::VideoComposer> composer_;

    protected:
        std::optional<std::pair<bytes::unique_binary, size_t>> idle_data() override;

        std::chrono::nanoseconds idle_interval() const override;

    public:
        explicit RawWriter(media::BaseSink* sink);

        void send_frames(std::vector<wrtc::models::Frame> frames) override;

        void remove_source(int64_t ssrc) override;
    };

} // ntgcalls::io
