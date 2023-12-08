//
// Created by Laky64 on 30/08/2023.
//

#pragma once

#ifdef BOOST_ENABLED
#include <boost/process.hpp>

#include "base_reader.hpp"
#include "../exceptions.hpp"

namespace bp = boost::process;

namespace ntgcalls {

    class ShellReader final: public BaseReader {
        bp::ipstream stdOut;
        bp::opstream stdIn;
        bp::child shellProcess;

        wrtc::binary readInternal(int64_t size) override;

    public:
        explicit ShellReader(const std::string& command);

        ~ShellReader() override;

        void close() override;
    };

} // ntgcalls
#endif