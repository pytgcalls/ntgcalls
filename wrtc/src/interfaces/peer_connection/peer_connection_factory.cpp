//
// Created by Laky64 on 16/08/2023.
//

#include <wrtc/interfaces/peer_connection/peer_connection_factory.hpp>
#include <api/enable_media.h>
#include <rtc_base/ssl_adapter.h>
#include <api/audio/builtin_audio_processing_builder.h>
#include <api/create_peerconnection_factory.h>
#include <api/rtc_event_log/rtc_event_log_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <pc/media_factory.h>
#include <system_wrappers/include/field_trial.h>
#include <wrtc/interfaces/media/audio_device_module.hpp>
#include <wrtc/utils/java_context.hpp>

#include <wrtc/video_factory/video_factory_config.hpp>
#include <wrtc/video_factory/hardware/android/video_factory.hpp>

namespace wrtc {
    std::mutex PeerConnectionFactory::_mutex{};
    bool PeerConnectionFactory::initialized = false;
    rtc::scoped_refptr<PeerConnectionFactory> PeerConnectionFactory::_default = nullptr;

    PeerConnectionFactory::PeerConnectionFactory() {
        webrtc::field_trial::InitFieldTrialsFromString(
            "WebRTC-DataChannel-Dcsctp/Enabled/"
            "WebRTC-Audio-MinimizeResamplingOnMobile/Enabled/"
            "WebRTC-Audio-iOS-Holding/Enabled/"
            "WebRTC-IceFieldTrials/skip_relay_to_non_relay_connections:true/"
        );
        network_thread_ = rtc::Thread::CreateWithSocketServer();
        network_thread_->SetName("ntg-net", nullptr);
        network_thread_->Start();
        worker_thread_ = rtc::Thread::Create();
        worker_thread_->SetName("ntg-work", nullptr);
        worker_thread_->Start();
        signaling_thread_ = rtc::Thread::Create();
        signaling_thread_->SetName("ntg-media", nullptr);
        signaling_thread_->Start();

        signaling_thread_->AllowInvokesToThread(worker_thread_.get());
        signaling_thread_->AllowInvokesToThread(network_thread_.get());
        worker_thread_->AllowInvokesToThread(network_thread_.get());

        webrtc::PeerConnectionFactoryDependencies dependencies;
        dependencies.network_thread = network_thread_.get();
        dependencies.worker_thread = worker_thread_.get();
        dependencies.signaling_thread = signaling_thread_.get();
        dependencies.task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
        dependencies.event_log_factory = std::make_unique<webrtc::RtcEventLogFactory>();
        jniEnv = GetJNIEnv();
        dependencies.adm = worker_thread_->BlockingCall([&] {
            if (!_audioDeviceModule)
                _audioDeviceModule = rtc::make_ref_counted<AudioDeviceModule>();
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
            factory_ = CreateModularPeerConnectionFactoryWithContext(std::move(dependencies), connection_context_);
        }
        webrtc::PeerConnectionFactoryInterface::Options options;
        options.crypto_options.srtp.enable_gcm_crypto_suites = true;
        options.crypto_options.srtp.enable_aes128_sha1_80_crypto_cipher = true;
        factory_->SetOptions(options);
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

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> PeerConnectionFactory::factory() {
        return factory_;
    }

    rtc::Thread* PeerConnectionFactory::networkThread() const {
        return network_thread_.get();
    }

    rtc::Thread* PeerConnectionFactory::signalingThread() const {
        return signaling_thread_.get();
    }

    rtc::Thread* PeerConnectionFactory::workerThread() const {
        return worker_thread_.get();
    }

    rtc::NetworkManager* PeerConnectionFactory::networkManager() const {
        return connection_context_->default_network_manager();
    }

    rtc::PacketSocketFactory* PeerConnectionFactory::socketFactory() const {
        return connection_context_->default_socket_factory();
    }

    rtc::UniqueRandomIdGenerator* PeerConnectionFactory::ssrcGenerator() const {
        return connection_context_->ssrc_generator();
    }

    cricket::MediaEngineInterface* PeerConnectionFactory::mediaEngine() const {
        return connection_context_->media_engine();
    }

    const webrtc::FieldTrialsView& PeerConnectionFactory::fieldTrials() const {
        return connection_context_->env().field_trials();
    }

    const webrtc::Environment& PeerConnectionFactory::environment() const {
        return connection_context_->env();
    }

    webrtc::MediaFactory* PeerConnectionFactory::mediaFactory() const {
        return connection_context_->call_factory();
    }

    std::vector<webrtc::SdpVideoFormat> PeerConnectionFactory::getSupportedVideoFormats() const {
        return supportedVideoFormats;
    }

    rtc::scoped_refptr<PeerConnectionFactory> PeerConnectionFactory::GetOrCreateDefault() {
        std::lock_guard lock(_mutex);
        if (initialized == false) {
#ifndef IS_ANDROID
            rtc::InitializeSSL();
#endif
            initialized = true;
            _default = rtc::scoped_refptr<PeerConnectionFactory>(new rtc::RefCountedObject<PeerConnectionFactory>());
        }
        return _default;
    }
} // wrtc