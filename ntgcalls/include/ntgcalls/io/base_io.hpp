//
// Created by Laky64 on 08/10/24.
//

#pragma once
#include <atomic>
#include <ntgcalls/media/base_sink.hpp>
#include <wrtc/utils/syncronized_callback.hpp>

namespace ntgcalls {

    class BaseIO {
    protected:
        BaseSink *sink = nullptr;
        std::atomic_bool running, exiting;
        wrtc::synchronized_callback<void> eofCallback;

    public:
        explicit BaseIO(BaseSink *sink);

        virtual ~BaseIO();

        void onEof(const std::function<void()>& callback);
    };

} // ntgcalls
