//
// Created by Laky64 on 30/08/2023.
//

#pragma once

#ifdef BOOST_ENABLED
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <ntgcalls/io/threaded_reader.hpp>

namespace bp = boost::process;
namespace asio = boost::asio;

namespace ntgcalls {

    class ShellReader final: public ThreadedReader {
        asio::io_context ctx;
        asio::readable_pipe stdOut{ctx};
        bp::process shellProcess{ctx};

    public:
        explicit ShellReader(const std::string& command, BaseSink *sink);

        ~ShellReader() override;

        void open() override;
    };

} // ntgcalls
#endif