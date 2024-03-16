//
// Created by Laky64 on 16/03/2024.
//

#pragma once

#include "signaling_interface.hpp"
#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {

    class Signaling {
    public:
        static std::shared_ptr<SignalingInterface> Create(
            const std::vector<std::string> &versions,
            rtc::Thread* networkThread,
            rtc::Thread* signalingThread,
            bool isOutGoing,
            const Key &key,
            const std::function<void(const bytes::binary&)>& onEmitData,
            const std::function<void(const std::optional<bytes::binary>&)>& onSignalData
        );
    };

} // ntgcalls
