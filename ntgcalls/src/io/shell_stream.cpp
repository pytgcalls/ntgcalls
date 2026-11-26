//
// Created by Lauren on 30/08/23.
//

#ifdef BOOST_ENABLED
#ifdef IS_WINDOWS
#include <boost/process/v2/windows/default_launcher.hpp>
#else
#include <csignal>
#include <unistd.h>
#include <boost/process/v2/posix/vfork_launcher.hpp>
#endif
#include <thread>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/shell_stream.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::io {

#ifdef IS_WINDOWS
    ShellStream::JobAssignment::JobAssignment(const HANDLE job): job_(job) {}

    bp::error_code ShellStream::JobAssignment::on_setup(bp::windows::default_launcher& launcher, const bp::filesystem::path&, const std::wstring&) const {
        if (job_) {
            launcher.creation_flags |= CREATE_SUSPENDED;
        }
        return {};
    }

    void ShellStream::JobAssignment::on_success(bp::windows::default_launcher& launcher, const bp::filesystem::path&, std::wstring&) const {
        if (!job_) {
            return;
        }
        AssignProcessToJobObject(job_, launcher.process_information.hProcess);
        ResumeThread(launcher.process_information.hThread);
    }
#else
    bp::error_code ShellStream::ProcessGroup::on_exec_setup(bp::posix::default_launcher&, const bp::filesystem::path&, const char* const*) {
        if (setpgid(0, 0) == -1) {
            return bp::detail::get_last_error();
        }
        return {};
    }
#endif

    ShellStream::ShellStream(const std::string& command, const Mode mode): mode_(mode) {
        try {
            const auto cmd = bp::shell(command);
            const asio::any_io_executor executor = ctx_.get_executor();
#ifdef IS_WINDOWS
            job_ = CreateJobObjectW(nullptr, nullptr);
            if (job_) {
                JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
                limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
                SetInformationJobObject(job_, JobObjectExtendedLimitInformation, &limits, sizeof(limits));
            }
            if (mode_ == Mode::Read) {
                shell_process_ = bp::process(executor, cmd.exe(), cmd.args(), bp::process_stdio{nullptr, std_out_, {}}, JobAssignment(job_));
            } else {
                shell_process_ = bp::process(executor, cmd.exe(), cmd.args(), bp::process_stdio{std_in_, nullptr, {}}, JobAssignment(job_));
            }
#else
            if (mode_ == Mode::Read) {
                shell_process_ = bp::posix::vfork_launcher()(executor, cmd.exe(), cmd.args(), bp::process_stdio{nullptr, std_out_, {}}, ProcessGroup());
            } else {
                shell_process_ = bp::posix::vfork_launcher()(executor, cmd.exe(), cmd.args(), bp::process_stdio{std_in_, nullptr, {}}, ProcessGroup());
            }
#endif
        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::system::errc::no_such_file_or_directory) {
                RTC_LOG(LS_ERROR) << "No executable found for the command: " << command;
                throw ShellError("No executable found for the command: " + command);
            }
            RTC_LOG(LS_ERROR) << "Failed to start the command: " << e.code().message();
            throw ShellError("Failed to start the command: " + e.code().message());
        } catch (std::runtime_error& e) {
            throw ShellError(e.what());
        }
    }

    ShellStream::~ShellStream() {
        release();
#ifdef IS_WINDOWS
        if (job_) {
            CloseHandle(job_);
        }
#endif
    }

    bytes::unique_binary ShellStream::read(const int64_t size) {
        auto data = bytes::make_unique_binary(size);
        boost::system::error_code ec;
        asio::read(std_out_, asio::buffer(data.get(), size), ec);
        if (ec) {
            RTC_LOG(LS_WARNING) << "Reached end of the stream";
            throw EOFError("Reached end of the stream");
        }
        return std::move(data);
    }

    void ShellStream::write(const bytes::unique_binary& data, const size_t size) {
        boost::system::error_code ec;
        asio::write(std_in_, asio::buffer(data.get(), size), ec);
        if (ec || !std_in_.is_open() || !shell_process_.running()) {
            throw EOFError("Reached end of the stream");
        }
    }

    void ShellStream::close() {
        release();
    }

    void ShellStream::release() {
        boost::system::error_code ec;
        if (std_in_.is_open()) {
            std_in_.close(ec);
            wait_exit();
        }
        terminate();
        if (std_out_.is_open()) {
            std_out_.close(ec);
        }
    }

    void ShellStream::interrupt() {
        terminate();
    }

    void ShellStream::wait_exit() {
        boost::system::error_code ec;
        const auto deadline = std::chrono::steady_clock::now() + kExitTimeout;
        while (shell_process_.running(ec) && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void ShellStream::terminate() {
        boost::system::error_code ec;
        if (!shell_process_.running(ec)) {
            return;
        }
#ifdef IS_WINDOWS
        if (job_) {
            TerminateJobObject(job_, 1);
        } else {
            shell_process_.terminate(ec);
        }
#else
        if (shell_process_.id() > 0) {
            kill(-shell_process_.id(), SIGKILL);
        }
#endif
        shell_process_.wait(ec);
        RTC_LOG(LS_VERBOSE) << "ShellStream closed";
    }

} // ntgcalls::io
#endif
