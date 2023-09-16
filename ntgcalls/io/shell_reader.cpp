//
// Created by Laky64 on 30/08/2023.
//

#include "shell_reader.hpp"

#ifdef BOOST_ENABLED
namespace ntgcalls {
    ntgcalls::ShellReader::ShellReader(const std::string &command) {
        try {
            shellProcess = bp::child(command, bp::std_out > stdOut, bp::std_in < stdIn);
        } catch (std::runtime_error &e) {
            throw ShellError(e.what());
        }
    }

    ShellReader::~ShellReader() {
        close();
    }

    wrtc::binary ShellReader::readInternal(size_t size) {
        if (stdOut.eof() || stdOut.fail() || !stdOut.is_open() || !shellProcess || !shellProcess.running()) {
            throw EOFError("Reached end of the stream");
        }
        auto *file_data = new uint8_t[size];
        stdOut.read(reinterpret_cast<char*>(file_data), size);
        if (stdOut.fail()) {
            throw FileError("Error while reading the file");
        }
        return file_data;
    }

    void ShellReader::close() {
        BaseReader::close();
        if (stdOut) {
            stdOut.close();
            stdOut.pipe().close();
        }
        if (stdIn) {
            stdIn.close();
            stdIn.pipe().close();
        }
        if (shellProcess) {
            shellProcess.terminate();
            shellProcess.wait();
            shellProcess.detach();
        }
    }
} // ntgcalls
#endif
