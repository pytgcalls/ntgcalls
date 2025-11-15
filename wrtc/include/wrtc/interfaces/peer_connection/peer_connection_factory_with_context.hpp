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
        static webrtc::scoped_refptr<PeerConnectionFactoryInterface> Create(
                const webrtc::Environment &env,
                webrtc::PeerConnectionFactoryDependencies dependencies,
                webrtc::scoped_refptr<webrtc::ConnectionContext>& context);

        explicit PeerConnectionFactoryWithContext(
                const webrtc::Environment &env,
                webrtc::PeerConnectionFactoryDependencies dependencies);

        PeerConnectionFactoryWithContext(
            const webrtc::Environment &env,
            const webrtc::scoped_refptr<webrtc::ConnectionContext>& context,
                webrtc::PeerConnectionFactoryDependencies* dependencies);

        static webrtc::scoped_refptr<PeerConnectionFactoryWithContext> Create(
            const webrtc::Environment &env,
            webrtc::PeerConnectionFactoryDependencies dependencies);

        [[nodiscard]] webrtc::scoped_refptr<webrtc::ConnectionContext> GetContext() const;

    private:
        webrtc::scoped_refptr<webrtc::ConnectionContext> conn_context_;
    };

    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> CreateModularPeerConnectionFactoryWithContext(
        const webrtc::Environment &env,
        webrtc::PeerConnectionFactoryDependencies dependencies,
        webrtc::scoped_refptr<webrtc::ConnectionContext>& context
    );
} // wrtc

