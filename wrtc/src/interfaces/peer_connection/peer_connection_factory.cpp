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
    std::unique_ptr<PeerConnectionFactory, void (*)(PeerConnectionFactory*)> PeerConnectionFactory::default_{nullptr, &PeerConnectionFactory::destroy_default};

    PeerConnectionFactory::PeerConnectionFactory() {
        av_log_set_level(AV_LOG_QUIET);
        network_thread_ = utils::SafeThread::CreateWithSocketServer();
        network_thread_->SetName("ntg-net", nullptr);
        network_thread_->Start();
        signaling_thread_ = utils::SafeThread::Create();
        signaling_thread_->SetName("ntg-media", nullptr);
        signaling_thread_->Start();

        signaling_thread_->AllowInvokesToThread(*network_thread_);

        webrtc::PeerConnectionFactoryDependencies dependencies;
        const auto env = environment();
        dependencies.network_thread = *network_thread_;
        dependencies.signaling_thread = *signaling_thread_;
        dependencies.env = env;
        dependencies.event_log_factory = std::make_unique<webrtc::RtcEventLogFactory>();
        jni_env_ = utils::get_jni_env();
        dependencies.adm = network_thread_->BlockingCall([&] {
            if (!audio_device_module_)
                audio_device_module_ = webrtc::make_ref_counted<media::AudioDeviceModule>();
            return audio_device_module_;
        });
        audio_encoder_factory_ = webrtc::make_ref_counted<audio_factory::AudioEncoderFactory>();
        dependencies.audio_encoder_factory = audio_encoder_factory_;
        audio_decoder_factory_ = webrtc::make_ref_counted<audio_factory::AudioDecoderFactory>();
        dependencies.audio_decoder_factory = audio_decoder_factory_;
#ifdef IS_ANDROID
        auto video_encoder_factory = android::create_video_encoder_factory(static_cast<JNIEnv*>(jni_env_));
        auto video_decoder_factory = android::create_video_decoder_factory(static_cast<JNIEnv*>(jni_env_));
#else
        auto config = video_factory::VideoFactoryConfig();
        std::unique_ptr<webrtc::VideoEncoderFactory> video_encoder_factory = config.create_video_encoder_factory();
        std::unique_ptr<webrtc::VideoDecoderFactory> video_decoder_factory = config.create_video_decoder_factory();
#endif
        auto passthrough_decoder_factory = std::make_unique<video_factory::PassthroughDecoderFactory>(std::move(video_decoder_factory));
        auto passthrough_encoder_factory = std::make_unique<video_factory::PassthroughEncoderFactory>(std::move(video_encoder_factory), passthrough_decoder_factory->inner());
        video_encoder_factory_ = passthrough_encoder_factory.get();
        video_decoder_factory_ = passthrough_decoder_factory.get();
        dependencies.video_encoder_factory = std::move(passthrough_encoder_factory);
        dependencies.video_decoder_factory = std::move(passthrough_decoder_factory);
        dependencies.audio_mixer = nullptr;
        supported_video_formats_ = dependencies.video_encoder_factory->GetSupportedFormats();
        EnableMedia(dependencies);
        if (!factory_) {
            factory_ = create_modular_peer_connection_factory_with_context(env, std::move(dependencies), connection_context_);
        }
    }

    PeerConnectionFactory::~PeerConnectionFactory() {
        if (audio_device_module_) {
            network_thread_->BlockingCall([this] {
                if (audio_device_module_)
                    audio_device_module_ = nullptr;
            });
        }
        factory_ = nullptr;
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
        const std::lock_guard lock(media_engine_mutex_);
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

    audio_factory::AudioEncoderFactory* PeerConnectionFactory::audio_encoder_factory() const {
        return audio_encoder_factory_.get();
    }

    audio_factory::AudioDecoderFactory* PeerConnectionFactory::audio_decoder_factory() const {
        return audio_decoder_factory_.get();
    }

    video_factory::PassthroughEncoderFactory* PeerConnectionFactory::video_encoder_factory() const {
        return video_encoder_factory_;
    }

    video_factory::PassthroughDecoderFactory* PeerConnectionFactory::video_decoder_factory() const {
        return video_decoder_factory_;
    }

    PeerConnectionFactory* PeerConnectionFactory::get_or_create_default() {
        const std::lock_guard lock(mutex_);
        if (initialized_ == false) {
#ifndef IS_ANDROID
            webrtc::InitializeSSL();
#endif
            initialized_ = true;
            default_.reset(new PeerConnectionFactory());
        }
        return default_.get();
    }

    void PeerConnectionFactory::destroy_default([[maybe_unused]] PeerConnectionFactory* factory) {
#ifndef IS_WINDOWS
        delete factory;
#endif
    }
} // wrtc::interfaces::peer_connection
