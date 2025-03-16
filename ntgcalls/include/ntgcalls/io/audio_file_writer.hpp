//
// Created by Laky64 on 07/10/24.
//

#pragma once

#include <fstream>
#include <string>
#include <ntgcalls/io/audio_mixer.hpp>
#include <ntgcalls/media/base_sink.hpp>
#include <ntgcalls/io/threaded_audio_mixer.hpp>

namespace ntgcalls {

    class AudioFileWriter final: public ThreadedAudioMixer {
        std::ofstream source;

    public:
        AudioFileWriter(const std::string& path, BaseSink* sink);

        ~AudioFileWriter() override;

    protected:
        void write(const bytes::unique_binary& data) override;
    };

} // ntgcalls
