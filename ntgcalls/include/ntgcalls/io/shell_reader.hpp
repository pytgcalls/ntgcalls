//
// Created by Lauren on 30/08/23.
//

#pragma once

#ifdef BOOST_ENABLED
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <ntgcalls/io/threaded_reader.hpp>

namespace bp = boost::process;
namespace asio = boost::asio;

namespace ntgcalls::io {

    class ShellReader final: public ThreadedReader {
#ifdef IS_WINDOWS
        class JobAssignment {
            HANDLE job_;

        public:
            explicit JobAssignment(HANDLE job);

            bp::error_code on_setup(bp::windows::default_launcher& launcher, const bp::filesystem::path&, const std::wstring&) const;

            void on_success(bp::windows::default_launcher& launcher, const bp::filesystem::path&, std::wstring&) const;
        };

        HANDLE job_ = nullptr;
#else
        class ProcessGroup {
        public:
            bp::error_code on_exec_setup(bp::posix::default_launcher&, const bp::filesystem::path&, const char* const*) const;
        };
#endif
        asio::io_context ctx_;
        asio::readable_pipe std_out_{ctx_};
        bp::process shell_process_{ctx_};

        void terminate_process_tree();

    public:
        explicit ShellReader(const std::string& command, media::BaseSink* sink);

        ~ShellReader() override;

        void open() override;
    };

} // ntgcalls::io
#endif
