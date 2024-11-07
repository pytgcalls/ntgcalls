//
// Created by Laky64 on 07/10/24.
//

#pragma once
#include <mutex>
#include <memory>
#include <wrtc/interfaces/media/remote_media_interface.hpp>

namespace ntgcalls {

    class BaseReceiver {
    protected:
        std::mutex mutex;
        std::weak_ptr<wrtc::RemoteMediaInterface> weakSink;

    public:
        virtual ~BaseReceiver() = default;

        virtual void open() = 0;
    };

} // ntgcalls
