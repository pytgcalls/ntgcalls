//
// Created by Lauren on 08/10/24.
//

#pragma once
#include <atomic>
#include <ntgcalls/media/base_sink.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace ntgcalls::io {

    class BaseIO {
    protected:
        media::BaseSink* sink_ = nullptr;
        std::atomic_bool running_;
        wrtc::utils::synchronized_callback<void()> eof_callback_;

    public:
        explicit BaseIO(media::BaseSink* sink);

        virtual ~BaseIO();

        void on_eof(const std::function<void()>& callback);
    };

} // ntgcalls::io
