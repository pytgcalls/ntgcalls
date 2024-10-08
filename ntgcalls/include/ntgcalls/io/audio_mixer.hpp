//
// Created by Laky64 on 07/10/24.
//

#pragma once

#include <ntgcalls/io/audio_writer.hpp>
#include <ntgcalls/media/base_sink.hpp>

namespace ntgcalls {

    class AudioMixer: public AudioWriter {
    protected:
        BaseSink* sink;

        virtual void onData(bytes::unique_binary data) = 0;

    public:
        explicit AudioMixer(BaseSink* sink);

        ~AudioMixer() override;

        void sendFrames(const std::map<uint32_t, bytes::unique_binary>& frames) override;
    };

} // ntgcalls
