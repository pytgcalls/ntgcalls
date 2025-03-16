//
// Created by Laky64 on 22/03/2024.
//

#include <wrtc/interfaces/peer_connection/data_channel_observer_impl.hpp>

namespace wrtc {
    DataChannelObserverImpl::DataChannelObserverImpl(Parameters&& parameters): parameters(std::move(parameters)) {}

    void DataChannelObserverImpl::OnStateChange() {
        if (parameters.onStateChange) {
            parameters.onStateChange();
        }
    }

    void DataChannelObserverImpl::OnMessage(webrtc::DataBuffer const& buffer) {
        if (parameters.onMessage) {
            parameters.onMessage(buffer);
        }
    }
} // wrtc