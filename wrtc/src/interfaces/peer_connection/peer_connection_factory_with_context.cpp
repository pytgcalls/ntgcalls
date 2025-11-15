//
// Created by Laky64 on 30/09/2023.
//

#include <wrtc/interfaces/peer_connection/peer_connection_factory_with_context.hpp>

namespace wrtc {
    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
    PeerConnectionFactoryWithContext::Create(
            const webrtc::Environment &env,
            webrtc::PeerConnectionFactoryDependencies dependencies,
            webrtc::scoped_refptr<webrtc::ConnectionContext> &context) {
        using result_type =
                std::pair<webrtc::scoped_refptr<PeerConnectionFactoryInterface>,
                        webrtc::scoped_refptr<webrtc::ConnectionContext>>;
        auto [fst, snd] = dependencies.signaling_thread->BlockingCall([&dependencies, env] {
            const auto factory =
                Create(env, std::move(dependencies));
            if (factory == nullptr) {
                return result_type(nullptr, nullptr);
            }
            auto connection_context = factory->GetContext();
            auto proxy = webrtc::PeerConnectionFactoryProxy::Create(
                    factory->signaling_thread(), factory->worker_thread(), factory);
            return result_type(proxy, connection_context);
        });
        context = snd;
        return fst;
    }

    webrtc::scoped_refptr<PeerConnectionFactoryWithContext>
    PeerConnectionFactoryWithContext::Create(const webrtc::Environment &env, webrtc::PeerConnectionFactoryDependencies dependencies) {
        return webrtc::make_ref_counted<PeerConnectionFactoryWithContext>(
                env,
                std::move(dependencies));
    }

    webrtc::scoped_refptr<webrtc::ConnectionContext> PeerConnectionFactoryWithContext::GetContext() const {
        return conn_context_;
    }

    PeerConnectionFactoryWithContext::PeerConnectionFactoryWithContext(
        const webrtc::Environment &env,
        const webrtc::scoped_refptr<webrtc::ConnectionContext>& context,
            webrtc::PeerConnectionFactoryDependencies *dependencies)
            : PeerConnectionFactory(env, context, dependencies),
              conn_context_(context) {}

    PeerConnectionFactoryWithContext::PeerConnectionFactoryWithContext(
            const webrtc::Environment &env,
            webrtc::PeerConnectionFactoryDependencies dependencies)
            : PeerConnectionFactoryWithContext(
                env,
                webrtc::ConnectionContext::Create(env, &dependencies),
            &dependencies) {}

    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> CreateModularPeerConnectionFactoryWithContext(
        const webrtc::Environment &env,
        webrtc::PeerConnectionFactoryDependencies dependencies,
        webrtc::scoped_refptr<webrtc::ConnectionContext>& context
    ) {
        using result_type =std::pair<webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>, webrtc::scoped_refptr<webrtc::ConnectionContext>>;
        auto [fst, snd] = dependencies.signaling_thread->BlockingCall([&dependencies, env] {
            const auto factory = PeerConnectionFactoryWithContext::Create(env, std::move(dependencies));
            if (factory == nullptr) {
              return result_type(nullptr, nullptr);
            }
            auto ctx = factory->GetContext();
            auto proxy = webrtc::PeerConnectionFactoryProxy::Create(factory->signaling_thread(), factory->worker_thread(), factory);
            return result_type(proxy, ctx);
        });
        context = snd;
        return fst;
    }
} // wrtc