//
// Created by Laky64 on 07/10/24.
//

#pragma once
#include <wrtc/utils/syncronized_callback.hpp>

namespace ntgcalls {

    class BaseWriter {
    protected:
        wrtc::synchronized_callback<void> eofCallback;

    public:
        virtual ~BaseWriter() = default;

        void onEof(const std::function<void()> &callback);

        virtual void open() = 0;
    };

} // ntgcalls
