//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <mutex>
#include <memory>
#include <wrtc/interfaces/media/remote_media_interface.hpp>

namespace ntgcalls::media {

    class BaseReceiver {
    protected:
        std::mutex mutex_;
        std::weak_ptr<wrtc::interfaces::media::RemoteMediaInterface> weakSink_;

    public:
        virtual ~BaseReceiver() = default;

        virtual void open() = 0;
    };

} // ntgcalls
