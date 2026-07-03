//
// Created by Lauren on 22/03/24.
//

#include <wrtc/interfaces/peer_connection/data_channel_observer_impl.hpp>

namespace wrtc::interfaces::peer_connection {
    DataChannelObserverImpl::DataChannelObserverImpl(Parameters&& parameters): parameters_(std::move(parameters)) {}

    DataChannelObserverImpl::~DataChannelObserverImpl() {
        parameters_.on_state_change = nullptr;
        parameters_.on_message = nullptr;
    }

    void DataChannelObserverImpl::OnStateChange() {
        if (parameters_.on_state_change) {
            parameters_.on_state_change();
        }
    }

    void DataChannelObserverImpl::OnMessage(webrtc::DataBuffer const& buffer) {
        if (parameters_.on_message) {
            parameters_.on_message(buffer);
        }
    }
} // wrtc::interfaces::peer_connection