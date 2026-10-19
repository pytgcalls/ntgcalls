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
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/shell_reader.hpp>

namespace ntgcalls::io {

#ifdef IS_WINDOWS
    ShellReader::JobAssignment::JobAssignment(const HANDLE job): job_(job) {}

    bp::error_code ShellReader::JobAssignment::on_setup(bp::windows::default_launcher& launcher, const bp::filesystem::path&, const std::wstring&) const {
        if (job_) {
            launcher.creation_flags |= CREATE_SUSPENDED;
        }
        return {};
    }

    void ShellReader::JobAssignment::on_success(bp::windows::default_launcher& launcher, const bp::filesystem::path&, std::wstring&) const {
        if (!job_) {
            return;
        }
        AssignProcessToJobObject(job_, launcher.process_information.hProcess);
        ResumeThread(launcher.process_information.hThread);
    }
#else
    bp::error_code ShellReader::ProcessGroup::on_exec_setup(bp::posix::default_launcher&, const bp::filesystem::path&, const char* const*) const {
        if (setpgid(0, 0) == -1) {
            return bp::detail::get_last_error();
        }
        return {};
    }
#endif

    ShellReader::ShellReader(const std::string& command, media::BaseSink* sink):
    BaseIO(sink), ThreadedReader(sink) {
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
            shell_process_ = bp::process(executor, cmd.exe(), cmd.args(), bp::process_stdio{nullptr, std_out_, {}}, JobAssignment(job_));
#else
            shell_process_ = bp::posix::vfork_launcher()(executor, cmd.exe(), cmd.args(), bp::process_stdio{nullptr, std_out_, {}}, ProcessGroup());
#endif
        } catch (std::runtime_error& e) {
            throw ShellError(e.what());
        }
    }

    ShellReader::~ShellReader() {
        terminate_process_tree();
        close();
        boost::system::error_code ec;
        if (std_out_.is_open()) {
            std_out_.close(ec);
        }
#ifdef IS_WINDOWS
        if (job_) {
            CloseHandle(job_);
        }
#endif
        RTC_LOG(LS_VERBOSE) << "ShellReader closed";
    }

    void ShellReader::terminate_process_tree() {
        boost::system::error_code ec;
#ifdef IS_WINDOWS
        if (job_) {
            TerminateJobObject(job_, 1);
        } else if (shell_process_.running(ec)) {
            shell_process_.terminate(ec);
        }
#else
        if (shell_process_.id() > 0) {
            ::kill(-shell_process_.id(), SIGKILL);
        }
#endif
        shell_process_.wait(ec);
    }

    void ShellReader::open() {
        run([this](const int64_t size) {
            auto file_data = bytes::make_unique_binary(size);
            boost::system::error_code ec;
            asio::read(std_out_, asio::buffer(file_data.get(), size), ec);
            if (ec) {
                RTC_LOG(LS_WARNING) << "Reached end of the file";
                throw EOFError("Reached end of the stream");
            }
            return std::move(file_data);
        });
    }
} // ntgcalls::io
#endif
