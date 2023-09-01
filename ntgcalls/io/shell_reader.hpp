//
// Created by Laky64 on 30/08/2023.
//

#pragma once

#include <boost/process.hpp>
#include <boost/asio.hpp>

#include "base_reader.hpp"
#include "../exceptions.hpp"

namespace bp = boost::process;
namespace asio = boost::asio;

namespace ntgcalls {

    class ShellReader final: public BaseReader {
    private:
        bp::ipstream stdOut;
        bp::child process;

        wrtc::binary readInternal(size_t size) override;

        bool eofInternal() override;

    public:
        ShellReader(const std::string& command);

        ~ShellReader() override;

        void close() final;
    };

} // ntgcalls
