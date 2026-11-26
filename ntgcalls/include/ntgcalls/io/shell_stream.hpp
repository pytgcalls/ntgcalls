//
// Created by Lauren on 30/08/23.
//

#pragma once

#ifdef BOOST_ENABLED
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <ntgcalls/io/base_stream.hpp>

namespace bp = boost::process;
namespace asio = boost::asio;

namespace ntgcalls::io {

    class ShellStream: public BaseStream {
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
            static bp::error_code on_exec_setup(bp::posix::default_launcher&, const bp::filesystem::path&, const char* const*);
        };
#endif
        asio::io_context ctx_;
        asio::readable_pipe std_out_{ctx_};
        asio::writable_pipe std_in_{ctx_};
        bp::process shell_process_{ctx_};
        Mode mode_;

        static constexpr auto kExitTimeout = std::chrono::seconds(2);

        void wait_exit();

        void terminate();

        void release();

    public:
        ShellStream(const std::string& command, Mode mode);

        ~ShellStream() override;

        bytes::unique_binary read(int64_t size) override;

        size_t read_some(uint8_t* data, size_t size) override;

        void write(const bytes::unique_binary& data, size_t size) override;

        void close() override;

        void interrupt() override;
    };

} // ntgcalls::io
#endif
