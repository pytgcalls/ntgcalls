//
// Created by Lauren on 30/09/23.
//

#pragma once

#include <api/environment/environment_factory.h>
#include <pc/peer_connection_factory.h>

namespace wrtc::interfaces::peer_connection {

    class PeerConnectionFactoryWithContext: public webrtc::PeerConnectionFactory {
    public:
        static webrtc::scoped_refptr<PeerConnectionFactoryInterface> Create(
            const webrtc::Environment& env,
            webrtc::PeerConnectionFactoryDependencies dependencies,
            webrtc::scoped_refptr<webrtc::ConnectionContext>& context
        );

        explicit PeerConnectionFactoryWithContext(
            const webrtc::Environment& env,
            webrtc::PeerConnectionFactoryDependencies dependencies
        );

        PeerConnectionFactoryWithContext(
            const webrtc::Environment& env,
            const webrtc::scoped_refptr<webrtc::ConnectionContext>& context,
            webrtc::PeerConnectionFactoryDependencies* dependencies
        );

        static webrtc::scoped_refptr<PeerConnectionFactoryWithContext> Create(
            const webrtc::Environment& env,
            webrtc::PeerConnectionFactoryDependencies dependencies
        );

        [[nodiscard]] webrtc::scoped_refptr<webrtc::ConnectionContext> get_context() const;

    private:
        webrtc::scoped_refptr<webrtc::ConnectionContext> conn_context_;
    };

    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> create_modular_peer_connection_factory_with_context(
        const webrtc::Environment& env,
        webrtc::PeerConnectionFactoryDependencies dependencies,
        webrtc::scoped_refptr<webrtc::ConnectionContext>& context
    );
} // wrtc::interfaces::peer_connection
