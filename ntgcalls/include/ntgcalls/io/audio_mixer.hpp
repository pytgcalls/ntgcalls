//
// Created by Lauren on 07/10/24.
//

#pragma once

#include <ntgcalls/io/audio_writer.hpp>
#include <ntgcalls/media/base_sink.hpp>

namespace ntgcalls::io {

    class AudioMixer: public AudioWriter {
    protected:
        virtual void on_data(bytes::unique_binary data) = 0;

    public:
        explicit AudioMixer(media::BaseSink* sink);

        void send_frames(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) override;
    };

} // ntgcalls::io
