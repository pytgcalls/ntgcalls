//
// Created by Laky64 on 28/09/24.
//

#pragma once
#include <ntgcalls/media/base_sink.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/syncronized_callback.hpp>

namespace ntgcalls {

    class BaseReader {
    protected:
        wrtc::synchronized_callback<void> eofCallback;
        wrtc::synchronized_callback<bytes::unique_binary> dataCallback;
        BaseSink *sink;

    public:
        explicit BaseReader(BaseSink *sink);

        virtual ~BaseReader() = default;

        virtual void open() = 0;

        virtual void close() = 0;

        void onEof(const std::function<void()> &callback);

        void onData(const std::function<void(bytes::unique_binary)> &callback);
    };

} // ntgcalls
