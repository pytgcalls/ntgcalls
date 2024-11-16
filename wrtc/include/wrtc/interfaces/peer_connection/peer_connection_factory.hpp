//
// Created by Laky64 on 16/08/2023.
//

#pragma once

#include <mutex>
#include <api/peer_connection_interface.h>
#include <wrtc/interfaces/peer_connection/peer_connection_factory_with_context.hpp>

namespace wrtc {

    class PeerConnectionFactory: public webrtc::RefCountInterface {
    public:
        PeerConnectionFactory();

        ~PeerConnectionFactory() override;

        static rtc::scoped_refptr<PeerConnectionFactory> GetOrCreateDefault();

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory();

        [[nodiscard]] rtc::Thread* networkThread() const;

        [[nodiscard]] rtc::Thread* signalingThread() const;

        [[nodiscard]] rtc::Thread* workerThread() const;

        [[nodiscard]] rtc::NetworkManager* networkManager() const;

        [[nodiscard]] rtc::PacketSocketFactory* socketFactory() const;

        [[nodiscard]] rtc::UniqueRandomIdGenerator* ssrcGenerator() const;

        [[nodiscard]] cricket::MediaEngineInterface* mediaEngine() const;

        [[nodiscard]] const webrtc::FieldTrialsView &fieldTrials() const;

        [[nodiscard]] const webrtc::Environment& environment() const;

        [[nodiscard]] webrtc::MediaFactory* mediaFactory() const;

        [[nodiscard]] std::vector<webrtc::SdpVideoFormat> getSupportedVideoFormats() const;

    private:
        static std::mutex _mutex;
        static bool initialized;
        void *jniEnv;
        static rtc::scoped_refptr<PeerConnectionFactory> _default;

        std::unique_ptr<rtc::Thread> network_thread_;
        std::unique_ptr<rtc::Thread> worker_thread_;
        std::unique_ptr<rtc::Thread> signaling_thread_;
        rtc::scoped_refptr<webrtc::ConnectionContext> connection_context_;

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
        rtc::scoped_refptr<webrtc::AudioDeviceModule> _audioDeviceModule;

        std::vector<webrtc::SdpVideoFormat> supportedVideoFormats;
    };

} // wrtc
