//
// Created by Laky64 on 30/08/2023.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/shell_reader.hpp>

#ifdef BOOST_ENABLED
namespace ntgcalls {

    ShellReader::ShellReader(const std::string &command, BaseSink *sink):
    BaseIO(sink), ThreadedReader(sink) {
        try {
            const auto cmd = bp::shell(command);
            shellProcess = bp::process(ctx, cmd.exe(), cmd.args(), bp::process_stdio{nullptr, stdOut, {}});
        } catch (std::runtime_error &e) {
            throw ShellError(e.what());
        }
    }

    ShellReader::~ShellReader() {
        close();
        boost::system::error_code ec;
        shellProcess.terminate(ec);
        shellProcess.wait(ec);
        shellProcess.detach();
        RTC_LOG(LS_VERBOSE) << "ShellReader closed";
    }

    void ShellReader::open() {
        run([this](const int64_t size) {
            auto file_data = bytes::make_unique_binary(size);
            boost::system::error_code ec;
            asio::read(stdOut, asio::buffer(file_data.get(), size), ec);
            if (ec || !stdOut.is_open() || !shellProcess.running()) {
                RTC_LOG(LS_WARNING) << "Reached end of the file";
                throw EOFError("Reached end of the stream");
            }
            return std::move(file_data);
        });
    }
} // ntgcalls
#endif
