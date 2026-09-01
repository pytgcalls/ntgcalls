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
        asio::io_context ctx_;
        asio::readable_pipe std_out_{ctx_};
        bp::process shell_process_{ctx_};

    public:
        explicit ShellReader(const std::string& command, media::BaseSink* sink);

        ~ShellReader() override;

        void open() override;
    };

} // ntgcalls::io
#endif
