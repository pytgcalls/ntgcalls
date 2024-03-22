//
// Created by Laky64 on 22/03/2024.
//

#pragma once
#include <api/data_channel_interface.h>

namespace wrtc {

    class DataChannelObserverImpl final : public webrtc::DataChannelObserver {
    public:
        struct Parameters {
            std::function<void()> onStateChange;
            std::function<void(webrtc::DataBuffer const &)> onMessage;
        };

        explicit DataChannelObserverImpl(Parameters &&parameters);

        void OnStateChange() override;

        void OnMessage(webrtc::DataBuffer const &buffer) override;

        ~DataChannelObserverImpl() override = default;

    private:
        Parameters parameters;
    };

} // wrtc
