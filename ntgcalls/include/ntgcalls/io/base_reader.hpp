//
// Created by Laky64 on 28/09/24.
//

#pragma once
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/syncronized_callback.hpp>
#include <ntgcalls/io/base_io.hpp>
#include <wrtc/models/frame_data.hpp>

namespace ntgcalls {

    class BaseReader: public virtual BaseIO {
    protected:
        wrtc::synchronized_callback<bytes::unique_binary, wrtc::FrameData> dataCallback;
        bool enabled = true;

    public:
        explicit BaseReader(BaseSink *sink);

        virtual void open() = 0;

        void onData(const std::function<void(bytes::unique_binary, wrtc::FrameData)> &callback);

        virtual bool set_enabled(bool status);

        bool is_enabled() const;
    };

} // ntgcalls
