//
// Created by Laky64 on 08/10/24.
//

#pragma once
#include <ntgcalls/io/threaded_audio_mixer.hpp>

#ifdef BOOST_ENABLED
#include <boost/process.hpp>

namespace bp = boost::process;

namespace ntgcalls {

    class AudioShellWriter final: public ThreadedAudioMixer {
        bp::child shellProcess;
        bp::opstream stdIn;

        void write(const bytes::unique_binary& data) override;

    public:
        explicit AudioShellWriter(const std::string &command, BaseSink* sink);

        ~AudioShellWriter() override;
    };

} // ntgcalls

#endif