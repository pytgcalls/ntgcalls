//
// Created by Laky64 on 16/08/2023.
//

#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>
#include <api/enable_media.h>
#include <api/field_trials.h>
#include <rtc_base/ssl_adapter.h>
#include <api/audio/builtin_audio_processing_builder.h>
#include <api/create_peerconnection_factory.h>
#include <api/rtc_event_log/rtc_event_log_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <pc/media_factory.h>
#include <wrtc/interfaces/media/audio_device_module.hpp>
#include <wrtc/utils/java_context.hpp>

extern "C" {
#include <libavutil/avutil.h>
}

#include <wrtc/video_factory/video_factory_config.hpp>
#include <wrtc/video_factory/hardware/android/video_factory.hpp>

namespace wrtc {
    std::mutex PeerConnectionFactory::_mutex{};
    bool PeerConnectionFactory::initialized = false;
    std::unique_ptr<PeerConnectionFactory> PeerConnectionFactory::_default = nullptr;

    PeerConnectionFactory::PeerConnectionFactory() {
        av_log_set_level(AV_LOG_QUIET);
        network_thread_ = webrtc::Thread::CreateWithSocketServer();
        network_thread_->SetName("ntg-net", nullptr);
        network_thread_->Start();
        worker_thread_ = webrtc::Thread::Create();
        worker_thread_->SetName("ntg-work", nullptr);
        worker_thread_->Start();
        signaling_thread_ = webrtc::Thread::Create();
        signaling_thread_->SetName("ntg-media", nullptr);
        signaling_thread_->Start();

        signaling_thread_->AllowInvokesToThread(worker_thread_.get());
        signaling_thread_->AllowInvokesToThread(network_thread_.get());
        worker_thread_->AllowInvokesToThread(network_thread_.get());

        webrtc::PeerConnectionFactoryDependencies dependencies;
        auto env = environment();
        dependencies.network_thread = network_thread_.get();
        dependencies.worker_thread = worker_thread_.get();
        dependencies.signaling_thread = signaling_thread_.get();
        dependencies.env = env;
        dependencies.event_log_factory = std::make_unique<webrtc::RtcEventLogFactory>(&env.task_queue_factory());
        jniEnv = GetJNIEnv();
        dependencies.adm = worker_thread_->BlockingCall([&] {
            if (!_audioDeviceModule)
                _audioDeviceModule = webrtc::make_ref_counted<AudioDeviceModule>();
            return _audioDeviceModule;
        });
        dependencies.audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
        dependencies.audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();
#ifdef IS_ANDROID
        dependencies.video_encoder_factory = android::CreateVideoEncoderFactory(static_cast<JNIEnv*>(jniEnv));
        dependencies.video_decoder_factory = android::CreateVideoDecoderFactory(static_cast<JNIEnv*>(jniEnv));
#else
        auto config = VideoFactoryConfig();
        dependencies.video_encoder_factory = config.CreateVideoEncoderFactory();
        dependencies.video_decoder_factory = config.CreateVideoDecoderFactory();
#endif
        dependencies.audio_mixer = nullptr;
        supportedVideoFormats = dependencies.video_encoder_factory->GetSupportedFormats();
        EnableMedia(dependencies);
        if (!factory_) {
            factory_ = CreateModularPeerConnectionFactoryWithContext(env, std::move(dependencies), connection_context_);
        }
    }

    PeerConnectionFactory::~PeerConnectionFactory() {
        if (_audioDeviceModule) {
            worker_thread_->BlockingCall([this]
            {
                if (_audioDeviceModule)
                    _audioDeviceModule = nullptr;
            });
        }
        factory_ = nullptr;
        worker_thread_->Stop();
        signaling_thread_->Stop();
        network_thread_->Stop();
    }

    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PeerConnectionFactory::factory() {
        return factory_;
    }

    webrtc::Thread* PeerConnectionFactory::networkThread() const {
        return network_thread_.get();
    }

    webrtc::Thread* PeerConnectionFactory::signalingThread() const {
        return signaling_thread_.get();
    }

    webrtc::Thread* PeerConnectionFactory::workerThread() const {
        return worker_thread_.get();
    }

    webrtc::NetworkManager* PeerConnectionFactory::networkManager() const {
        return connection_context_->default_network_manager();
    }

    webrtc::PacketSocketFactory* PeerConnectionFactory::socketFactory() const {
        return connection_context_->default_socket_factory();
    }

    webrtc::UniqueRandomIdGenerator* PeerConnectionFactory::ssrcGenerator() const {
        return connection_context_->ssrc_generator();
    }

    webrtc::MediaEngineInterface* PeerConnectionFactory::mediaEngine() {
        if (!media_engine_ref) {
            media_engine_ref = std::make_unique<webrtc::ConnectionContext::MediaEngineReference>(
                webrtc::scoped_refptr(connection_context_)
            );
        }
        return media_engine_ref->media_engine();
    }

    webrtc::Environment PeerConnectionFactory::environment() {
        return webrtc::CreateEnvironment(
            webrtc::FieldTrials::Create(
                "WebRTC-DataChannel-Dcsctp/Enabled/"
                "WebRTC-Audio-MinimizeResamplingOnMobile/Enabled/"
                "WebRTC-Audio-iOS-Holding/Enabled/"
                "WebRTC-IceFieldTrials/skip_relay_to_non_relay_connections:true/"
            )
        );
    }

    webrtc::MediaFactory* PeerConnectionFactory::mediaFactory() const {
        return connection_context_->call_factory();
    }

    std::vector<webrtc::SdpVideoFormat> PeerConnectionFactory::getSupportedVideoFormats() const {
        return supportedVideoFormats;
    }

    PeerConnectionFactory* PeerConnectionFactory::GetOrCreateDefault() {
        std::lock_guard lock(_mutex);
        if (initialized == false) {
#ifndef IS_ANDROID
            webrtc::InitializeSSL();
#endif
            initialized = true;
            _default = std::make_unique<PeerConnectionFactory>();
        }
        return _default.get();
    }
} // wrtc