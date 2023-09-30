//
// Created by Laky64 on 16/08/2023.
//

#pragma once

#include <mutex>
#include <rtc_base/ssl_adapter.h>
#include <api/peer_connection_interface.h>
#include <api/create_peerconnection_factory.h>
#include <api/rtc_event_log/rtc_event_log_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <media/engine/webrtc_media_engine.h>
#include "peer_connection_factory_with_context.hpp"
#include "../../video_factory/video_factory_config.hpp"
#include "pc/connection_context.h"

namespace wrtc {

    class PeerConnectionFactory: public rtc::RefCountInterface {
    public:
        PeerConnectionFactory();

        ~PeerConnectionFactory();

        static rtc::scoped_refptr<PeerConnectionFactory> GetOrCreateDefault();

        static void UnRef();

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory();
    private:
        void DestroyAudioDeviceModule_w();

        static std::mutex _mutex;
        static int _references;
        static rtc::scoped_refptr<PeerConnectionFactory> _default;

        std::unique_ptr<rtc::Thread> network_thread_;
        std::unique_ptr<rtc::Thread> worker_thread_;
        std::unique_ptr<rtc::Thread> signaling_thread_;

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _factory;
        rtc::scoped_refptr<webrtc::ConnectionContext> connection_context_;
        rtc::scoped_refptr<webrtc::AudioDeviceModule> _audioDeviceModule;

        std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory_;
    };

} // wrtc
