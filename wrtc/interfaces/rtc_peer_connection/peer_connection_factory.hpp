//
// Created by Laky64 on 08/08/2023.
//

#pragma once

#include <mutex>

#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/scoped_refptr.h>
#include <webrtc/modules/audio_device/include/audio_device.h>
#include <api/create_peerconnection_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <p2p/base/basic_packet_socket_factory.h>
#include <rtc_base/ssl_adapter.h>


namespace rtc {

  class NetworkManager;

  class PacketSocketFactory;

  class Thread;

}  // namespace rtc

namespace webrtc {

  class PeerConnectionFactoryInterface;

}  // namespace webrtc

namespace wrtc {

    class PeerConnectionFactory {
    public:
        explicit PeerConnectionFactory();

        ~PeerConnectionFactory();

        static PeerConnectionFactory *GetOrCreateDefault();

        static void Release();

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory() { return _factory; }

        rtc::NetworkManager *getNetworkManager() { return _networkManager.get(); }

        rtc::PacketSocketFactory *getSocketFactory() { return _socketFactory.get(); }

        static void Dispose();

        std::unique_ptr<rtc::Thread> _signalingThread;
        std::unique_ptr<rtc::Thread> _workerThread;

    private:
        static PeerConnectionFactory *_default;
        static std::mutex _mutex;
        static int _references;

        rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _factory;
        rtc::scoped_refptr<webrtc::AudioDeviceModule> _audioDeviceModule;

        std::unique_ptr<rtc::NetworkManager> _networkManager;
        std::unique_ptr<rtc::PacketSocketFactory> _socketFactory;
    };

} // namespace wrtc
