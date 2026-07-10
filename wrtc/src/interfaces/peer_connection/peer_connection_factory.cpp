//
// Created by Lauren on 16/08/23.
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

namespace wrtc::interfaces::peer_connection {
    std::mutex PeerConnectionFactory::mutex_{};
    bool PeerConnectionFactory::initialized_ = false;
    std::unique_ptr<PeerConnectionFactory> PeerConnectionFactory::default_ = nullptr;

    PeerConnectionFactory::PeerConnectionFactory() {
        av_log_set_level(AV_LOG_QUIET);
        network_thread_ = utils::SafeThread::CreateWithSocketServer();
        network_thread_->SetName("ntg-net", nullptr);
        network_thread_->Start();
        worker_thread_ = utils::SafeThread::Create();
        worker_thread_->SetName("ntg-work", nullptr);
        worker_thread_->Start();
        signaling_thread_ = utils::SafeThread::Create();
        signaling_thread_->SetName("ntg-media", nullptr);
        signaling_thread_->Start();

        signaling_thread_->AllowInvokesToThread(*worker_thread_);
        signaling_thread_->AllowInvokesToThread(*network_thread_);
        worker_thread_->AllowInvokesToThread(*network_thread_);

        webrtc::PeerConnectionFactoryDependencies dependencies;
        auto env = environment();
        dependencies.network_thread = *network_thread_;
        dependencies.worker_thread = *worker_thread_;
        dependencies.signaling_thread = *signaling_thread_;
        dependencies.env = env;
        dependencies.event_log_factory = std::make_unique<webrtc::RtcEventLogFactory>();
        jni_env_ = utils::GetJNIEnv();
        dependencies.adm = worker_thread_->BlockingCall([&] {
            if (!audio_device_module_)
                audio_device_module_ = webrtc::make_ref_counted<media::AudioDeviceModule>();
            return audio_device_module_;
        });
        dependencies.audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
        dependencies.audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();
#ifdef IS_ANDROID
        dependencies.video_encoder_factory = android::create_video_encoder_factory(static_cast<JNIEnv*>(jni_env_));
        dependencies.video_decoder_factory = android::create_video_decoder_factory(static_cast<JNIEnv*>(jni_env_));
#else
        auto config = video_factory::VideoFactoryConfig();
        dependencies.video_encoder_factory = config.CreateVideoEncoderFactory();
        dependencies.video_decoder_factory = config.CreateVideoDecoderFactory();
#endif
        dependencies.audio_mixer = nullptr;
        supported_video_formats_ = dependencies.video_encoder_factory->GetSupportedFormats();
        EnableMedia(dependencies);
        if (!factory_) {
            factory_ = create_modular_peer_connection_factory_with_context(env, std::move(dependencies), connection_context_);
        }
    }

    PeerConnectionFactory::~PeerConnectionFactory() {
        if (audio_device_module_) {
            worker_thread_->BlockingCall([this] {
                if (audio_device_module_)
                    audio_device_module_ = nullptr;
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

    utils::SafeThread& PeerConnectionFactory::network_thread() const {
        return *network_thread_;
    }

    utils::SafeThread& PeerConnectionFactory::signaling_thread() const {
        return *signaling_thread_;
    }

    utils::SafeThread& PeerConnectionFactory::worker_thread() const {
        return *worker_thread_;
    }

    webrtc::NetworkManager* PeerConnectionFactory::network_manager() const {
        return connection_context_->default_network_manager();
    }

    webrtc::PacketSocketFactory* PeerConnectionFactory::socket_factory() const {
        return connection_context_->default_socket_factory();
    }

    webrtc::UniqueRandomIdGenerator* PeerConnectionFactory::ssrc_generator() const {
        return connection_context_->ssrc_generator();
    }

    webrtc::MediaEngineInterface* PeerConnectionFactory::media_engine() {
        if (!media_engine_ref_) {
            media_engine_ref_ = std::make_unique<webrtc::ConnectionContext::MediaEngineReference>(
                webrtc::scoped_refptr(connection_context_)
            );
        }
        return media_engine_ref_->media_engine();
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

    webrtc::MediaFactory* PeerConnectionFactory::media_factory() const {
        return connection_context_->call_factory();
    }

    std::vector<webrtc::SdpVideoFormat> PeerConnectionFactory::get_supported_video_formats() const {
        return supported_video_formats_;
    }

    PeerConnectionFactory* PeerConnectionFactory::get_or_create_default() {
        const std::lock_guard lock(mutex_);
        if (initialized_ == false) {
#ifndef IS_ANDROID
            webrtc::InitializeSSL();
#endif
            initialized_ = true;
            default_ = std::make_unique<PeerConnectionFactory>();
        }
        return default_.get();
    }
} // wrtc::interfaces::peer_connection
