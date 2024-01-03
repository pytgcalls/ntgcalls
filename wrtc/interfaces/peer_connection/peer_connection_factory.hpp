//
// Created by Laky64 on 16/08/2023.
//

#pragma once

#include <mutex>
#include <api/peer_connection_interface.h>
#include <media/engine/webrtc_media_engine.h>
#include "pc/connection_context.h"

namespace wrtc {

    class PeerConnectionFactory: public rtc::RefCountInterface {
    public:
        PeerConnectionFactory();

        ~PeerConnectionFactory() override;

        static rtc::scoped_refptr<PeerConnectionFactory> GetOrCreateDefault();

        static void UnRef();

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory();
    private:
        static std::mutex _mutex;
        static int _references;
        static rtc::scoped_refptr<PeerConnectionFactory> _default;

        std::unique_ptr<rtc::Thread> network_thread_;
        std::unique_ptr<rtc::Thread> worker_thread_;
        std::unique_ptr<rtc::Thread> signaling_thread_;

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
        rtc::scoped_refptr<webrtc::ConnectionContext> connection_context_;
        rtc::scoped_refptr<webrtc::AudioDeviceModule> _audioDeviceModule;
    };

} // wrtc
