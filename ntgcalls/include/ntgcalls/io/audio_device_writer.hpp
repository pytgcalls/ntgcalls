//
// Created by Lauren on 07/10/24.
//

#pragma once

#include <ntgcalls/io/base_writer.hpp>
#include <ntgcalls/media/base_sink.hpp>

namespace ntgcalls::io {

    class AudioDeviceWriter: public BaseWriter {
    protected:
        virtual void on_data(bytes::unique_binary data, size_t size) = 0;

    public:
        explicit AudioDeviceWriter(media::BaseSink* sink);

        void send_frames(std::vector<wrtc::models::Frame> frames) override;

    };

} // ntgcalls::io
