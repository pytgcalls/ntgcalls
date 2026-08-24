//
// Created by Lauren on 08/10/24.
//

#pragma once

#ifdef BOOST_ENABLED
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <ntgcalls/io/threaded_audio_mixer.hpp>

namespace bp = boost::process;
namespace asio = boost::asio;

namespace ntgcalls::io {

    class AudioShellWriter final: public ThreadedAudioMixer {
        asio::io_context ctx_;
        asio::writable_pipe std_in_{ctx_};
        bp::process shell_process_{ctx_};

    protected:
        void write(const bytes::unique_binary& data) override;

    public:
        explicit AudioShellWriter(const std::string& command, media::BaseSink* sink);

        ~AudioShellWriter() override;
    };

} // ntgcalls::io

#endif
