//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <ntgcalls/io/threaded_writer.hpp>

namespace ntgcalls::io {

    class RawWriter: public ThreadedWriter {
    protected:
        std::optional<std::pair<bytes::unique_binary, size_t>> idle_data() override;

    public:
        explicit RawWriter(media::BaseSink* sink);

        void send_frames(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) override;
    };

} // ntgcalls::io
