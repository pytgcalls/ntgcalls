//
// Created by Lauren on 22/03/24.
//

#pragma once
#include <api/data_channel_interface.h>

namespace wrtc::interfaces::peer_connection {

    class DataChannelObserverImpl final : public webrtc::DataChannelObserver {
    public:
        struct Parameters {
            std::function<void()> on_state_change;
            std::function<void(webrtc::DataBuffer const &)> on_message;
        };

        explicit DataChannelObserverImpl(Parameters &&parameters);

        void OnStateChange() override;

        void OnMessage(webrtc::DataBuffer const &buffer) override;

        ~DataChannelObserverImpl() override;

    private:
        Parameters parameters_;
    };

} // wrtc::interfaces::peer_connection
