//
// Created by Lauren on 08/10/24.
//

#ifdef BOOST_ENABLED
#ifndef IS_WINDOWS
#include <boost/process/v2/posix/vfork_launcher.hpp>
#endif
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/audio_shell_writer.hpp>

namespace ntgcalls::io {
    AudioShellWriter::AudioShellWriter(const std::string& command, media::BaseSink* sink): BaseIO(sink), ThreadedAudioMixer(sink) {
        try {
            const auto cmd = bp::shell(command);
            const asio::any_io_executor executor = ctx_.get_executor();
#ifdef IS_WINDOWS
            shell_process_ = bp::process(executor, cmd.exe(), cmd.args(), bp::process_stdio{std_in_, nullptr, {}});
#else
            shell_process_ = bp::posix::vfork_launcher()(executor, cmd.exe(), cmd.args(), bp::process_stdio{std_in_, nullptr, {}});
#endif
        } catch (std::runtime_error& e) {
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
