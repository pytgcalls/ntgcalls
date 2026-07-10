//
// Created by Lauren on 16/08/23.
//

#pragma once

#include <mutex>
#include <api/peer_connection_interface.h>
#include <wrtc/interfaces/peer_connection/peer_connection_factory_with_context.hpp>
#include <wrtc/utils/safe_thread.hpp>

namespace wrtc::interfaces::peer_connection {

    class PeerConnectionFactory {
    public:
        PeerConnectionFactory();

        ~PeerConnectionFactory();

        static PeerConnectionFactory* get_or_create_default();

        webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory();

        [[nodiscard]] utils::SafeThread& network_thread() const;

        [[nodiscard]] utils::SafeThread& signaling_thread() const;

        [[nodiscard]] utils::SafeThread& worker_thread() const;

        [[nodiscard]] webrtc::NetworkManager* network_manager() const;

        [[nodiscard]] webrtc::PacketSocketFactory* socket_factory() const;

        [[nodiscard]] webrtc::UniqueRandomIdGenerator* ssrc_generator() const;

        [[nodiscard]] webrtc::MediaEngineInterface* media_engine();

        static webrtc::Environment environment();

        [[nodiscard]] webrtc::MediaFactory* media_factory() const;

        [[nodiscard]] std::vector<webrtc::SdpVideoFormat> get_supported_video_formats() const;

    private:
        static std::mutex mutex_;
        static bool initialized_;
        void* jni_env_;
        static std::unique_ptr<PeerConnectionFactory> default_;

        std::unique_ptr<utils::SafeThread> network_thread_;
        std::unique_ptr<utils::SafeThread> worker_thread_;
        std::unique_ptr<utils::SafeThread> signaling_thread_;
        std::unique_ptr<webrtc::ConnectionContext::MediaEngineReference> media_engine_ref_ RTC_GUARDED_BY(worker_thread_);

        webrtc::scoped_refptr<webrtc::ConnectionContext> connection_context_;

        webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
        webrtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_module_;

        std::vector<webrtc::SdpVideoFormat> supported_video_formats_;
    };

} // wrtc::interfaces::peer_connection
