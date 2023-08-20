//
// Created by Laky64 on 16/08/2023.
//

#pragma once

#include <mutex>
#include <rtc_base/ssl_adapter.h>
#include <api/peer_connection_interface.h>
#include <api/create_peerconnection_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>

#include "../../video_factory/video_factory_config.hpp"

namespace wrtc {

    class PeerConnectionFactory: public rtc::RefCountInterface {
    public:
        PeerConnectionFactory();

        ~PeerConnectionFactory();

        static rtc::scoped_refptr<PeerConnectionFactory> GetOrCreateDefault();

        static void UnRef();

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory();

        std::unique_ptr<rtc::Thread> _workerThread;
    private:
        void CreateAudioDeviceModule_w();

        void DestroyAudioDeviceModule_w();

        static std::mutex _mutex;
        static int _references;
        static rtc::scoped_refptr<PeerConnectionFactory> _default;

        std::unique_ptr<rtc::Thread> _signalingThread;
        std::unique_ptr<rtc::Thread> _networkThread;

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _factory;
        rtc::scoped_refptr<webrtc::AudioDeviceModule> _audioDeviceModule;

        std::unique_ptr<webrtc::TaskQueueFactory> _taskQueueFactory;
    };

} // wrtc
