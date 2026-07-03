//
// Created by Lauren on 07/10/24.
//

#pragma once

#include <fstream>
#include <string>
#include <ntgcalls/io/audio_mixer.hpp>
#include <ntgcalls/io/threaded_audio_mixer.hpp>
#include <ntgcalls/media/base_sink.hpp>

namespace ntgcalls::io {

    class AudioFileWriter final: public ThreadedAudioMixer {
        std::ofstream source_;

    public:
        AudioFileWriter(const std::string& path, media::BaseSink* sink);

        ~AudioFileWriter() override;

    protected:
        void write(const bytes::unique_binary& data) override;
    };

} // ntgcalls::io
