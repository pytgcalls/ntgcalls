//
// Created by Laky64 on 30/08/2023.
//

#include "shell_reader.hpp"

#ifdef BOOST_ENABLED
namespace ntgcalls {
    ShellReader::ShellReader(const std::string &command, const int64_t bufferSize, const bool noLatency): BaseReader(bufferSize, noLatency) {
        try {
            shellProcess = bp::child(command, bp::std_out > stdOut, bp::std_in.close());
        } catch (std::runtime_error &e) {
            throw ShellError(e.what());
        }
    }

    ShellReader::~ShellReader() {
        close();
        stdOut.clear();
    }

    bytes::unique_binary ShellReader::readInternal(const int64_t size) {
        if (!stdOut || stdOut.eof() || stdOut.fail() || !stdOut.is_open()) {
            RTC_LOG(LS_WARNING) << "Reached end of the file";
            throw EOFError("Reached end of the stream");
        }
        auto file_data = bytes::make_unique_binary(size);
        stdOut.read(reinterpret_cast<char*>(file_data.get()), size);
        return std::move(file_data);
    }

    void ShellReader::close() {
        BaseReader::close();
        if (shellProcess) {
            shellProcess.terminate();
            shellProcess.wait();
            shellProcess.detach();
        }
        RTC_LOG(LS_VERBOSE) << "ShellReader closed";
    }
} // ntgcalls
#endif
