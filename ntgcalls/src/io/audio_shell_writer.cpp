//
// Created by Lauren on 08/10/24.
//

#ifdef BOOST_ENABLED
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/audio_shell_writer.hpp>

namespace ntgcalls::io {
    AudioShellWriter::AudioShellWriter(const std::string &command, media::BaseSink* sink): BaseIO(sink), ThreadedAudioMixer(sink) {
        try {
            const auto cmd = bp::shell(command);
            shell_process_ = bp::process(ctx_, cmd.exe(), cmd.args(), bp::process_stdio{std_in_, nullptr, {}});
        } catch (std::runtime_error &e) {
            throw ShellError(e.what());
        }
    }

    AudioShellWriter::~AudioShellWriter() {
        boost::system::error_code ec;
        if (std_in_.is_open()) {
            std_in_.close(ec);
        }
        if (shell_process_.running(ec)) {
            shell_process_.terminate(ec);
            shell_process_.wait(ec);
        }
    }

    void AudioShellWriter::write(const bytes::unique_binary& data) {
        boost::system::error_code ec;
        asio::write(std_in_, asio::buffer(data.get(), sink_->frame_size()), ec);
        if (ec || !std_in_.is_open() || !shell_process_.running()) {
            throw EOFError("Reached end of the stream");
        }
    }
} // ntgcalls::io

#endif