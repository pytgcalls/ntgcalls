//
// Created by Lauren on 28/09/24.
//

#pragma once
#include <ntgcalls/io/base_io.hpp>
#include <wrtc/models/frame_data.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace ntgcalls::io {

    class BaseReader: public virtual BaseIO {
    protected:
        wrtc::utils::synchronized_callback<void(bytes::unique_binary, wrtc::models::FrameData)> data_callback_;
        bool status_ = true;

    public:
        explicit BaseReader(media::BaseSink *sink);

        ~BaseReader() override;

        virtual void open() = 0;

        void on_data(const std::function<void(bytes::unique_binary, wrtc::models::FrameData)> &callback);

        virtual bool set_enabled(bool enable);

        bool is_enabled() const;
    };

} // ntgcalls::io
