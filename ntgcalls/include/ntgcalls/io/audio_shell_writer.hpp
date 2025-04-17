//
// Created by Laky64 on 08/10/24.
//

#pragma once

#ifdef BOOST_ENABLED
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <ntgcalls/io/threaded_audio_mixer.hpp>

namespace bp = boost::process;
namespace asio = boost::asio;

namespace ntgcalls {

    class AudioShellWriter final: public ThreadedAudioMixer {
        asio::io_context ctx;
        asio::writable_pipe stdIn{ctx};
        bp::process shellProcess{ctx};

        void write(const bytes::unique_binary& data) override;

    public:
        explicit AudioShellWriter(const std::string &command, BaseSink* sink);

        ~AudioShellWriter() override;
    };

} // ntgcalls

#endif