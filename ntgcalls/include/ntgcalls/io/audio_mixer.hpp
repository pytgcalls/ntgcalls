//
// Created by Lauren on 07/10/24.
//

#pragma once

#include <ntgcalls/io/base_writer.hpp>
#include <ntgcalls/media/base_sink.hpp>

namespace ntgcalls::io {

    class AudioMixer: public BaseWriter {
    protected:
        virtual void on_data(bytes::unique_binary data, size_t size) = 0;

    public:
        explicit AudioMixer(media::BaseSink* sink);

        void send_frames(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) override;

        static bytes::unique_binary mix(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames, int64_t frame_size);
    };

} // ntgcalls::io
