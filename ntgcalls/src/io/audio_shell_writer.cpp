//
// Created by Laky64 on 08/10/24.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/audio_shell_writer.hpp>

#ifdef BOOST_ENABLED

namespace ntgcalls {
    AudioShellWriter::AudioShellWriter(const std::string &command, BaseSink* sink): BaseIO(sink), ThreadedAudioMixer(sink) {
        try {
            shellProcess = bp::child(command, bp::std_out.close(), bp::std_in < stdIn);
        } catch (std::runtime_error &e) {
            throw ShellError(e.what());
        }
    }

    AudioShellWriter::~AudioShellWriter() {
        if (shellProcess) {
            shellProcess.terminate();
            shellProcess.wait();
            shellProcess.detach();
        }
        stdIn.clear();
    }

    void AudioShellWriter::write(const bytes::unique_binary& data) {
        if (!stdIn || stdIn.eof() || stdIn.fail() || !stdIn.pipe().is_open() || !shellProcess.running()) {
            throw EOFError("Reached end of the stream");
        }
        stdIn.write(reinterpret_cast<const char*>(data.get()), sink->frameSize());
        stdIn.flush();
    }
} // ntgcalls

#endif