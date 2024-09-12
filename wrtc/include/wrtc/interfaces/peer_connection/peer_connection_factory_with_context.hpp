//
// Created by Laky64 on 30/09/2023.
//

#pragma once

#include <api/environment/environment_factory.h>
#include <pc/peer_connection_factory.h>
#include <pc/peer_connection_factory_proxy.h>

namespace wrtc {

    class PeerConnectionFactoryWithContext : public webrtc::PeerConnectionFactory {
    public:
        static rtc::scoped_refptr<PeerConnectionFactoryInterface> Create(
                webrtc::PeerConnectionFactoryDependencies dependencies,
                rtc::scoped_refptr<webrtc::ConnectionContext>& context);

        explicit PeerConnectionFactoryWithContext(
                webrtc::PeerConnectionFactoryDependencies dependencies);

        PeerConnectionFactoryWithContext(
            const rtc::scoped_refptr<webrtc::ConnectionContext>& context,
                webrtc::PeerConnectionFactoryDependencies* dependencies);

        static rtc::scoped_refptr<PeerConnectionFactoryWithContext> Create(
        webrtc::PeerConnectionFactoryDependencies dependencies);

        [[nodiscard]] rtc::scoped_refptr<webrtc::ConnectionContext> GetContext() const;

    private:
        rtc::scoped_refptr<webrtc::ConnectionContext> conn_context_;
    };

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> CreateModularPeerConnectionFactoryWithContext(
        webrtc::PeerConnectionFactoryDependencies dependencies,
        rtc::scoped_refptr<webrtc::ConnectionContext>& context
    );
} // wrtc

