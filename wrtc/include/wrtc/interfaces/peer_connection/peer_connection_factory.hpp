//
// Created by Laky64 on 16/08/2023.
//

#pragma once

#include <mutex>
#include <api/peer_connection_interface.h>
#include <wrtc/interfaces/peer_connection/peer_connection_factory_with_context.hpp>

namespace wrtc {

    class PeerConnectionFactory {
    public:
        PeerConnectionFactory();

        ~PeerConnectionFactory();

        static PeerConnectionFactory* GetOrCreateDefault();

        webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory();

        [[nodiscard]] webrtc::Thread* networkThread() const;

        [[nodiscard]] webrtc::Thread* signalingThread() const;

        [[nodiscard]] webrtc::Thread* workerThread() const;

        [[nodiscard]] webrtc::NetworkManager* networkManager() const;

        [[nodiscard]] webrtc::PacketSocketFactory* socketFactory() const;

        [[nodiscard]] webrtc::UniqueRandomIdGenerator* ssrcGenerator() const;

        [[nodiscard]] webrtc::MediaEngineInterface* mediaEngine();

        static webrtc::Environment environment();

        [[nodiscard]] webrtc::MediaFactory* mediaFactory() const;

        [[nodiscard]] std::vector<webrtc::SdpVideoFormat> getSupportedVideoFormats() const;

    private:
        static std::mutex _mutex;
        static bool initialized;
        void *jniEnv;
        static std::unique_ptr<PeerConnectionFactory> _default;

        std::unique_ptr<webrtc::Thread> network_thread_;
        std::unique_ptr<webrtc::Thread> worker_thread_;
        std::unique_ptr<webrtc::Thread> signaling_thread_;
        std::unique_ptr<webrtc::ConnectionContext::MediaEngineReference> media_engine_ref RTC_GUARDED_BY(worker_thread_);

        webrtc::scoped_refptr<webrtc::ConnectionContext> connection_context_;

        webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
        webrtc::scoped_refptr<webrtc::AudioDeviceModule> _audioDeviceModule;

        std::vector<webrtc::SdpVideoFormat> supportedVideoFormats;
    };

} // wrtc
