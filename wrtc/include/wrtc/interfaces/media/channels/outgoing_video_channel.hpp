//
// Created by Lauren on 02/04/24.
//

#pragma once
#include <call/call.h>
#include <wrtc/interfaces/media/channel_manager.hpp>
#include <wrtc/interfaces/media/frame_transformer.hpp>
#include <wrtc/interfaces/media/local_video_adapter.hpp>
#include <wrtc/models/media_content.hpp>
#include <wrtc/models/video_encoding.hpp>
#include <wrtc/video_factory/passthrough/passthrough_encoder_factory.hpp>

namespace wrtc::interfaces::media::channels {
    class OutgoingVideoChannel {
        uint32_t ssrc_ = 0;
        std::unique_ptr<webrtc::BaseChannel> channel_;
        utils::SafeThread& network_thread_;
        std::unique_ptr<webrtc::VideoBitrateAllocatorFactory> bitrate_allocator_factory_;
        LocalVideoAdapter* sink_;
        models::VideoEncoding encoding_;
        video_factory::PassthroughEncoderFactory* encoder_factory_;
        std::optional<uint64_t> queue_id_;

    public:
        OutgoingVideoChannel(
            webrtc::Call* call,
            ChannelManager* channel_manager,
            video_factory::PassthroughEncoderFactory* encoder_factory,
            webrtc::RtpTransport* rtp_transport,
            const models::MediaContent& media_content,
            utils::SafeThread& network_thread,
            LocalVideoAdapter* sink,
            const std::map<int32_t, FrameTransformer::PayloadType>& payload_type_mapping,
            E2EEncryptor* encryptor,
            const models::VideoEncoding& encoding
        );

        ~OutgoingVideoChannel();

        void set_enabled(bool enable) const;

        [[nodiscard]] uint32_t ssrc() const;

        [[nodiscard]] models::VideoEncoding encoding() const;
    };
} // wrtc::interfaces::media::channels
