//
// Created by Laky64 on 30/08/2023.
//

#pragma once

#ifdef BOOST_ENABLED
#include <boost/process.hpp>

#include <ntgcalls/io/threaded_reader.hpp>

namespace bp = boost::process;

namespace ntgcalls {

    class ShellReader final: public ThreadedReader {
        bp::ipstream stdOut;
        bp::child shellProcess;

        bytes::unique_binary read(int64_t size) override;

    public:
        explicit ShellReader(const std::string& command, BaseSink *sink);

        ~ShellReader() override;
    };

} // ntgcalls
#endif