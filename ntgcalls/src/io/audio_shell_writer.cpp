//
// Created by Laky64 on 08/10/24.
//

#ifdef BOOST_ENABLED
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/audio_shell_writer.hpp>

namespace ntgcalls {
    AudioShellWriter::AudioShellWriter(const std::string &command, BaseSink* sink): BaseIO(sink), ThreadedAudioMixer(sink) {
        try {
            const auto cmd = bp::shell(command);
            shellProcess = bp::process(ctx, cmd.exe(), cmd.args(), bp::process_stdio{stdIn, nullptr, {}});
        } catch (std::runtime_error &e) {
            throw ShellError(e.what());
        }
    }

    AudioShellWriter::~AudioShellWriter() {
        boost::system::error_code ec;
        if (stdIn.is_open()) {
            ec = stdIn.close(ec);
        }
        shellProcess.terminate(ec);
        shellProcess.wait(ec);
        shellProcess.detach();
    }

    void AudioShellWriter::write(const bytes::unique_binary& data) {
        boost::system::error_code ec;
        asio::write(stdIn, asio::buffer(data.get(), sink->frameSize()), ec);
        if (ec || !stdIn.is_open() || !shellProcess.running()) {
            throw EOFError("Reached end of the stream");
        }
    }
} // ntgcalls

#endif