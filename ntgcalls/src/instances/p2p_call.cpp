//
// Created by Lauren on 15/03/24.
//

#include <ntgcalls/instances/p2p_call.hpp>

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/signaling/crypto/mod_exp_first.hpp>
#include <ntgcalls/signaling/messages/candidates_message.hpp>
#include <ntgcalls/signaling/messages/initial_setup_message.hpp>
#include <ntgcalls/signaling/messages/media_state_message.hpp>
#include <ntgcalls/signaling/messages/message.hpp>
#include <ntgcalls/signaling/messages/negotiate_channels_message.hpp>
#include <ntgcalls/utils/emoji_fingerprint.hpp>
#include <wrtc/interfaces/native_connection.hpp>
#include <wrtc/utils/encryption.hpp>

namespace ntgcalls::instances {
    void P2PCall::stop() {
        on_emit_data_ = nullptr;
        CallInterface::stop();
        if (signaling_) {
            signaling_->close();
            signaling_ = nullptr;
        }
    }

    void P2PCall::init() const {
        RTC_LOG(LS_INFO) << "Initializing P2P call";
        stream_manager_->enable_video_simulcast(false);
        stream_manager_->set_stream_sources(media::StreamManager::Mode::Capture);
        stream_manager_->set_stream_sources(media::StreamManager::Mode::Playback);
        RTC_LOG(LS_INFO) << "AVStream settings applied";
    }

    bytes::binary P2PCall::init_exchange(const p2p::DhConfig& dh_config, const std::optional<bytes::binary>& ga_hash) {
        if (ga_or_gb_) {
            RTC_LOG(LS_ERROR) << "Exchange already initialized";
            throw ConnectionError("Exchange already initialized");
        }
        auto first = signaling::crypto::ModExpFirst(dh_config.g, dh_config.p, dh_config.random);
        if (first.modexp.empty()) {
            RTC_LOG(LS_ERROR) << "Invalid modexp";
            throw CryptoError("Invalid modexp");
        }
        random_power_ = std::move(first.random_power);
        prime_ = dh_config.p;
        if (ga_hash) {
            ga_hash_ = ga_hash;
        }
        ga_or_gb_ = std::move(first.modexp);
        RTC_LOG(LS_INFO) << "P2P call initialized";
        return ga_hash ? ga_or_gb_.value() : openssl::Sha256::digest(ga_or_gb_.value());
    }

    p2p::AuthParams P2PCall::exchange_keys(const bytes::binary &ga_or_gb, const int64_t fingerprint) {
        if (connection_) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        if (!ga_or_gb_) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionNotFound("Connection not initialized");
        }
        if (key_) {
            RTC_LOG(LS_ERROR) << "Key already exchanged";
            throw ConnectionError("Key already exchanged");
        }
        if (ga_hash_) {
            if (!fingerprint) {
                RTC_LOG(LS_ERROR) << "Fingerprint not found";
                throw InvalidParams("Fingerprint not found");
            }
            if (ga_hash_ != openssl::Sha256::digest(ga_or_gb)) {
                RTC_LOG(LS_ERROR) << "Hash mismatch";
                throw CryptoError("Hash mismatch");
            }
        }
        const auto computed_auth_key = signaling::crypto::AuthKey::create_auth_key(
            ga_or_gb,
            random_power_,
            prime_
        );
        if (computed_auth_key.empty()) {
            RTC_LOG(LS_ERROR) << "Could not create auth key";
            throw CryptoError("Could not create auth key");
        }
        signaling::crypto::RawKey auth_key;
        signaling::crypto::AuthKey::fill_data(auth_key, computed_auth_key);
        const auto computed_fingerprint = signaling::crypto::AuthKey::fingerprint(auth_key);
        if (ga_hash_ && computed_fingerprint != static_cast<uint64_t>(fingerprint)) {
            RTC_LOG(LS_ERROR) << "Fingerprint mismatch";
            throw CryptoError("Fingerprint mismatch");
        }
        key_ = auth_key;
        RTC_LOG(LS_INFO) << "Key exchanged, fingerprint: " << computed_fingerprint;

        const auto& ga = ga_hash_ ? ga_or_gb : ga_or_gb_.value();
        bytes::binary fingerprint_data = computed_auth_key;
        fingerprint_data.insert(fingerprint_data.end(), ga.begin(), ga.end());
        const auto digest = openssl::Sha256::digest(bytes::view(fingerprint_data));
        fingerprint_emojis_ = utils::EmojiFingerprint::from_hash(digest);
        (void) update_emojis_callback_(fingerprint_emojis_);
        return p2p::AuthParams{
            static_cast<int64_t>(computed_fingerprint),
            ga_or_gb_.value(),
        };
    }

    void P2PCall::skip_exchange(bytes::binary encryption_key, const bool is_outgoing) {
        if (connection_) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        if (!skip_exchange_key_.empty()) {
            RTC_LOG(LS_ERROR) << "Key already exchanged";
            throw ConnectionError("Key already exchanged");
        }
        skip_exchange_key_ = std::move(encryption_key);
        skip_is_outgoing_ = is_outgoing;
        RTC_LOG(LS_VERBOSE) << "Exchange skipped";
    }

    void P2PCall::connect(const std::vector<p2p::RTCServer>& servers, const std::vector<std::string>& versions, const bool p2p_allowed, const std::optional<std::string> &custom_parameters) {
        RTC_LOG(LS_INFO) << "Connecting to P2P call, p2pAllowed: " << (p2p_allowed ? "true" : "false");
        if (connection_) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        auto encryption_key = std::make_shared<std::array<uint8_t, signaling::crypto::EncryptionKey::kSize>>();
        if (skip_exchange_key_.empty()) {
            if (!ga_or_gb_ || !key_) {
                RTC_LOG(LS_ERROR) << "Connection not initialized";
                throw ConnectionNotFound("Connection not initialized");
            }
            std::memcpy(encryption_key->data(), key_.value().data(), signaling::crypto::EncryptionKey::kSize);
        } else {
            std::memcpy(encryption_key->data(), skip_exchange_key_.data(), signaling::crypto::EncryptionKey::kSize);
        }
        wrtc::utils::json custom_parameters_json;
        if (custom_parameters) {
            custom_parameters_json = wrtc::utils::json::parse(*custom_parameters);
        }
        protocol_version_ = signaling::Signaling::match_version(versions);
        const std::weak_ptr weak(shared_from_this());
        connection_ = std::make_shared<wrtc::interfaces::NativeConnection>(
            p2p::RTCServer::to_rtc_servers(servers),
            p2p_allowed,
            type() == Type::Outgoing,
            custom_parameters_json
        );
        connection_->open();
        stream_manager_->optimize_sources(connection_.get());
        signaling_ = signaling::Signaling::create(
            protocol_version_,
            connection_->network_thread(),
            connection_->signaling_thread(),
            connection_->environment(),
            signaling::crypto::EncryptionKey(std::move(encryption_key), type() == Type::Outgoing),
            [weak](const bytes::binary &data) {
                const auto strong = std::static_pointer_cast<P2PCall>(weak.lock());
                if (!strong) {
                    return;
                }
                (void) strong->on_emit_data_(data);
            },
            [weak](const std::vector<bytes::binary> &data) {
                const auto strong = std::static_pointer_cast<P2PCall>(weak.lock());
                if (!strong) {
                    return;
                }
                strong->connection_->signaling_thread().PostTask([weak, data] {
                    const auto strong = std::static_pointer_cast<P2PCall>(weak.lock());
                    if (!strong) {
                        return;
                    }
                    for (const auto &packet : data) {
                        strong->process_signaling_data(packet);
                    }
                });
            }
        );
        connection_->on_ice_candidate([weak](const wrtc::models::IceCandidate& candidate) {
            const auto strong = std::static_pointer_cast<P2PCall>(weak.lock());
            if (!strong) {
                return;
            }
            signaling::messages::CandidatesMessage cand_mess;
            cand_mess.ice_candidates.push_back({
                candidate.sdp,
                candidate.mid,
                candidate.m_line
            });
            const auto message = cand_mess.serialize();
            RTC_LOG(LS_VERBOSE) << "Sending candidate: " << bytes::to_string(message);
            strong->signaling_->send(message);
        });
        connection_->on_data_channel_opened([weak] {
            const auto strong = std::static_pointer_cast<P2PCall>(weak.lock());
            if (!strong) {
                return;
            }
            strong->send_media_state(strong->stream_manager_->get_state());
            RTC_LOG(LS_VERBOSE) << "Data channel opened";
        });
        connection_->on_data_channel_message([weak](const bytes::binary& data) {
            const auto strong = std::static_pointer_cast<P2PCall>(weak.lock());
            if (!strong) {
                return;
            }
            strong->connection_->signaling_thread().PostTask([weak, data] {
                const auto strong = std::static_pointer_cast<P2PCall>(weak.lock());
                if (!strong) {
                    return;
                }
                strong->process_signaling_data(data);
            });
        });
        stream_manager_->add_track(media::StreamManager::Mode::Capture, media::StreamManager::Device::Microphone, connection_.get());
        stream_manager_->add_track(media::StreamManager::Mode::Capture, media::StreamManager::Device::Camera, connection_.get());
        stream_manager_->add_track(media::StreamManager::Mode::Capture, media::StreamManager::Device::Screen, connection_.get());
        stream_manager_->add_track(media::StreamManager::Mode::Playback, media::StreamManager::Device::Microphone, connection_.get());
        stream_manager_->add_track(media::StreamManager::Mode::Playback, media::StreamManager::Device::Camera, connection_.get());
        stream_manager_->add_track(media::StreamManager::Mode::Playback, media::StreamManager::Device::Screen, connection_.get());
        stream_manager_->on_upgrade([weak] (const media::MediaState media_state) {
            const auto strong = std::static_pointer_cast<P2PCall>(weak.lock());
            if (!strong) {
                return;
            }
            strong->send_media_state(media_state);
        });
        if (type() == Type::Outgoing) {
            send_initial_setup();
            send_offer_if_needed();
        }
        set_connection_observer(connection_);
    }

    void P2PCall::process_signaling_data(const bytes::binary& buffer) {
        if (signaling_ == nullptr) {
            return;
        }
        RTC_LOG(LS_VERBOSE) << "processSignalingData: " << std::string(buffer.begin(), buffer.end());
        try {
            switch (signaling::messages::Message::type(buffer)) {
            case signaling::messages::Message::Type::InitialSetup: {
                const auto message = signaling::messages::InitialSetupMessage::deserialize(buffer);
                wrtc::models::PeerIceParameters remote_ice_parameters;
                remote_ice_parameters.ufrag = message->ufrag;
                remote_ice_parameters.pwd = message->pwd;
                remote_ice_parameters.supports_renomination = message->supports_renomination;

                std::unique_ptr<webrtc::SSLFingerprint> fingerprint;
                std::string ssl_setup;
                if (!message->fingerprints.empty()) {
                    fingerprint = webrtc::SSLFingerprint::CreateUniqueFromRfc4572(message->fingerprints[0].hash, message->fingerprints[0].fingerprint);
                    ssl_setup = message->fingerprints[0].setup;
                }
                safe<wrtc::interfaces::NativeConnection>(connection_)->set_remote_params(remote_ice_parameters, std::move(fingerprint), ssl_setup);
                handshake_completed_ = true;
                if (type() == Type::Incoming) {
                    send_initial_setup();
                }
                apply_pending_ice_candidates();
                break;
            }
            case signaling::messages::Message::Type::Candidates: {
                for (const auto message = signaling::messages::CandidatesMessage::deserialize(buffer); const auto&[sdpString, sdpMid, sdpMLineIndex] : message->ice_candidates) {
                    webrtc::SdpParseError error;
                    std::unique_ptr<webrtc::IceCandidate> parse_candidate = webrtc::IceCandidate::Create(
                        sdpMid,
                        sdpMLineIndex,
                        sdpString,
                        &error
                    );
                    if (!parse_candidate) {
                        RTC_LOG(LS_ERROR) << "Failed to parse ICE candidate: " << error.description;
                        continue;
                    }
                    pending_ice_candidates_.emplace_back(
                        parse_candidate->sdp_mid(),
                        parse_candidate->sdp_mline_index(),
                        parse_candidate->ToString()
                    );
                }
                if (handshake_completed_) {
                    apply_pending_ice_candidates();
                }
                break;
            }
            case signaling::messages::Message::Type::NegotiateChannels: {
                const auto message = signaling::messages::NegotiateChannelsMessage::deserialize(buffer);
                auto negotiation_contents = std::make_unique<wrtc::interfaces::ContentNegotiationContext::NegotiationContents>();
                negotiation_contents->exchange_id = message->exchange_id;
                negotiation_contents->contents = message->contents;
                auto negotiation = message->serialize();
                if (const auto response = safe<wrtc::interfaces::NativeConnection>(connection_)->set_pending_answer(std::move(negotiation_contents))) {
                    signaling::messages::NegotiateChannelsMessage channel_message;
                    channel_message.exchange_id = response->exchange_id;
                    channel_message.contents = response->contents;
                    RTC_LOG(LS_VERBOSE) << "Sending negotiate channels: " << bytes::to_string(channel_message.serialize());
                    signaling_->send(channel_message.serialize());
                }
                send_offer_if_needed();
                safe<wrtc::interfaces::NativeConnection>(connection_)->create_channels();
                break;
            }
            case signaling::messages::Message::Type::MediaState: {
                const auto message = signaling::messages::MediaStateMessage::deserialize(buffer);
                const auto camera_state = parse_video_state(message->video_state);
                const auto screen_state = parse_video_state(message->screencast_state);
                const auto mic_state = message->is_muted ? media::StreamManager::Status::Idling : media::StreamManager::Status::Active;
                if (last_camera_state_ != camera_state) {
                    last_camera_state_ = camera_state;
                    (void) remote_source_callback_({0, camera_state, media::StreamManager::Device::Camera});
                }
                if (last_screen_state_ != screen_state) {
                    last_screen_state_ = screen_state;
                    (void) remote_source_callback_({0, screen_state, media::StreamManager::Device::Screen});
                }
                if (last_mic_state_ != mic_state) {
                    last_mic_state_ = mic_state;
                    (void) remote_source_callback_({0, mic_state, media::StreamManager::Device::Microphone});
                }
                break;
            }
            default:
                break;
            }
        } catch (InvalidParams& e) {
            RTC_LOG(LS_ERROR) << "Invalid params: " << e.what();
        }
    }

    void P2PCall::apply_pending_ice_candidates() {
        if (pending_ice_candidates_.empty()) {
            return;
        }
        for (const auto& candidate : pending_ice_candidates_) {
            connection_->add_ice_candidate(candidate);
        }
        pending_ice_candidates_.clear();
    }

    void P2PCall::send_media_state(const media::MediaState media_state) const {
        if (!connection_ -> is_data_channel_open()) {
            return;
        }
        signaling::messages::MediaStateMessage message;
        message.is_muted = media_state.muted;

        if (!stream_manager_->has_device(media::StreamManager::Capture, media::StreamManager::Camera)) {
            message.video_state = signaling::messages::MediaStateMessage::VideoState::Inactive;
        } else if (media_state.videoPaused) {
            message.video_state = signaling::messages::MediaStateMessage::VideoState::Suspended;
        } else {
            message.video_state = signaling::messages::MediaStateMessage::VideoState::Active;
        }

        if (!stream_manager_->has_device(media::StreamManager::Capture, media::StreamManager::Screen)) {
            message.screencast_state = signaling::messages::MediaStateMessage::VideoState::Inactive;
        } else if (media_state.presentationPaused) {
            message.screencast_state = signaling::messages::MediaStateMessage::VideoState::Suspended;
        } else {
            message.screencast_state = signaling::messages::MediaStateMessage::VideoState::Active;
        }

        RTC_LOG(LS_VERBOSE) << "Sending media state: " << bytes::to_string(message.serialize());
        connection_->send_data_channel_message(message.serialize());
    }

    void P2PCall::send_offer_if_needed() const {
        if (signaling_ == nullptr) {
            return;
        }
        if (const auto offer = safe<wrtc::interfaces::NativeConnection>(connection_)->get_pending_offer()) {
            signaling::messages::NegotiateChannelsMessage data;
            data.exchange_id = offer->exchange_id;
            data.contents = offer->contents;
            RTC_LOG(LS_VERBOSE) << "Sending offer: " << bytes::to_string(data.serialize());
            signaling_->send(data.serialize());
        }
    }

    void P2PCall::send_initial_setup() const {
        const std::weak_ptr weak(shared_from_this());
        connection_->network_thread().PostTask([weak] {
            const auto strong = std::static_pointer_cast<const P2PCall>(weak.lock());
            if (!strong) {
                return;
            }
            const auto connection = safe<wrtc::interfaces::NativeConnection>(strong->connection_);
            const auto local_fingerprint = connection->local_fingerprint();
            std::string hash;
            std::string fingerprint;
            if (local_fingerprint) {
                hash = local_fingerprint->algorithm;
                fingerprint = local_fingerprint->GetRfc4572Fingerprint();
            }
            std::string setup;
            if (strong->type() == Type::Outgoing) {
                setup = "actpass";
            } else {
                setup = "passive";
            }
            const auto local_ice_params = connection->local_ice_parameters();
            connection->signaling_thread().PostTask([weak, local_ice_params, hash, fingerprint, setup] {
                const auto strong_message = std::static_pointer_cast<const P2PCall>(weak.lock());
                if (!strong_message) {
                    return;
                }
                signaling::messages::InitialSetupMessage message;
                message.ufrag = local_ice_params.ufrag;
                message.pwd = local_ice_params.pwd;
                message.supports_renomination = local_ice_params.supports_renomination;
                signaling::messages::InitialSetupMessage::DtlsFingerprint dtls_fingerprint;
                dtls_fingerprint.hash = hash;
                dtls_fingerprint.fingerprint = fingerprint;
                dtls_fingerprint.setup = setup;
                message.fingerprints.push_back(std::move(dtls_fingerprint));
                
                const auto serialized_message = message.serialize();
                RTC_LOG(LS_VERBOSE) << "Sending initial setup: " << bytes::to_string(serialized_message);
                strong_message->signaling_->send(serialized_message);
            });
        });
    }

    void P2PCall::on_signaling_data(const std::function<void(const bytes::binary&)>& callback) {
        on_emit_data_ = callback;
    }

    void P2PCall::on_update_emojis(const std::function<void(std::string)> &callback) {
        update_emojis_callback_ = callback;
    }

    void P2PCall::send_signaling_data(const bytes::binary& buffer) const {
        if (!signaling_) {
            throw ConnectionError("Connection not initialized");
        }
        signaling_->receive(buffer);
    }

    CallInterface::Type P2PCall::type() const {
        if (skip_exchange_key_.empty()) {
            if (ga_or_gb_) {
                if (ga_hash_) {
                    return Type::Incoming;
                }
                return Type::Outgoing;
            }
        } else {
            return skip_is_outgoing_ ? Type::Outgoing : Type::Incoming;
        }
        return Type::P2P;
    }

    std::string P2PCall::get_fingerprint_emojis() {
        return fingerprint_emojis_;
    }
} // ntgcalls::instances