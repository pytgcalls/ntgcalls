//
// Created by Lauren on 30/08/23.
//

#ifdef BOOST_ENABLED
#ifndef IS_WINDOWS
#include <boost/process/v2/posix/vfork_launcher.hpp>
#endif
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/shell_reader.hpp>

namespace ntgcalls::io {

    ShellReader::ShellReader(const std::string& command, media::BaseSink* sink):
    BaseIO(sink), ThreadedReader(sink) {
        try {
            const auto cmd = bp::shell(command);
            const asio::any_io_executor executor = ctx_.get_executor();
#ifdef IS_WINDOWS
            shell_process_ = bp::process(executor, cmd.exe(), cmd.args(), bp::process_stdio{nullptr, std_out_, {}});
#else
            shell_process_ = bp::posix::vfork_launcher()(executor, cmd.exe(), cmd.args(), bp::process_stdio{nullptr, std_out_, {}});
#endif
        } catch (std::runtime_error& e) {
            throw ShellError(e.what());
        }
    }

    ShellReader::~ShellReader() {
        boost::system::error_code ec;
        if (shell_process_.running(ec)) {
            shell_process_.terminate(ec);
            shell_process_.wait(ec);
        }
        if (std_out_.is_open()) {
            std_out_.close(ec);
        }
        close();
        RTC_LOG(LS_VERBOSE) << "ShellReader closed";
    }

    void ShellReader::open() {
        run([this](const int64_t size) {
            auto file_data = bytes::make_unique_binary(size);
            boost::system::error_code ec;
            asio::read(std_out_, asio::buffer(file_data.get(), size), ec);
            if (ec || !std_out_.is_open() || !shell_process_.running()) {
                RTC_LOG(LS_WARNING) << "Reached end of the file";
                throw EOFError("Reached end of the stream");
            }
            return std::move(file_data);
        });
    }
} // ntgcalls::io
#endif
