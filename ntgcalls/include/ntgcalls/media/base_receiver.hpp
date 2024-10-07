//
// Created by Laky64 on 07/10/24.
//

#pragma once
#include <wrtc/interfaces/media/remote_media_interface.hpp>

namespace ntgcalls {

    class BaseReceiver {
    public:
        virtual ~BaseReceiver() = default;

        virtual wrtc::RemoteMediaInterface* remoteSink() = 0;
    };

} // ntgcalls
