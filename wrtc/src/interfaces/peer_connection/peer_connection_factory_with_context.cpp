//
// Created by Laky64 on 30/09/2023.
//

#include <wrtc/interfaces/peer_connection/peer_connection_factory_with_context.hpp>

namespace wrtc {
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
    PeerConnectionFactoryWithContext::Create(
            webrtc::PeerConnectionFactoryDependencies dependencies,
            rtc::scoped_refptr<webrtc::ConnectionContext> &context) {
        using result_type =
                std::pair<rtc::scoped_refptr<PeerConnectionFactoryInterface>,
                        rtc::scoped_refptr<webrtc::ConnectionContext>>;
        auto [fst, snd] = dependencies.signaling_thread->BlockingCall([&dependencies] {
            const auto factory =
                Create(std::move(dependencies));
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

    rtc::scoped_refptr<PeerConnectionFactoryWithContext>
    PeerConnectionFactoryWithContext::Create(webrtc::PeerConnectionFactoryDependencies dependencies) {
        return rtc::make_ref_counted<PeerConnectionFactoryWithContext>(
                std::move(dependencies));
    }

    rtc::scoped_refptr<webrtc::ConnectionContext> PeerConnectionFactoryWithContext::GetContext() const {
        return conn_context_;
    }

    PeerConnectionFactoryWithContext::PeerConnectionFactoryWithContext(
        const rtc::scoped_refptr<webrtc::ConnectionContext>& context,
            webrtc::PeerConnectionFactoryDependencies *dependencies)
            : PeerConnectionFactory(context, dependencies),
              conn_context_(context) {}

    PeerConnectionFactoryWithContext::PeerConnectionFactoryWithContext(
            webrtc::PeerConnectionFactoryDependencies dependencies)
            : PeerConnectionFactoryWithContext(
            webrtc::ConnectionContext::Create(webrtc::CreateEnvironment(), &dependencies),
            &dependencies) {}

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> CreateModularPeerConnectionFactoryWithContext(
        webrtc::PeerConnectionFactoryDependencies dependencies,
        rtc::scoped_refptr<webrtc::ConnectionContext>& context
    ) {
        using result_type =std::pair<rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>, rtc::scoped_refptr<webrtc::ConnectionContext>>;
        auto [fst, snd] = dependencies.signaling_thread->BlockingCall([&dependencies]() {
            const auto factory = PeerConnectionFactoryWithContext::Create(std::move(dependencies));
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