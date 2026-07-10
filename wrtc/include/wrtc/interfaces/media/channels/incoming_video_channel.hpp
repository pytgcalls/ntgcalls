//
// Created by Lauren on 25/10/24.
//

#pragma once
#include <pc/channel.h>
#include <pc/rtp_transport.h>
#include <wrtc/interfaces/media/channel_manager.hpp>
#include <wrtc/interfaces/media/raw_video_sink.hpp>
#include <wrtc/interfaces/media/remote_video_sink.hpp>
#include <wrtc/models/media_content.hpp>

namespace wrtc::interfaces::media::channels {

    class IncomingVideoChannel {
        uint32_t ssrc_ = 0;
        std::unique_ptr<webrtc::BaseChannel> channel_;
        std::unique_ptr<webrtc::VideoBitrateAllocatorFactory> video_bitrate_allocator_factory_;
        utils::SafeThread& worker_thread_;
        utils::SafeThread& network_thread_;
        std::unique_ptr<RawVideoSink> sink_;

    public:
        IncomingVideoChannel(
            webrtc::Call* call,
            ChannelManager* channel_manager,
            webrtc::RtpTransport* rtp_transport,
            std::vector<models::SsrcGroup> ssrc_groups,
            webrtc::UniqueRandomIdGenerator* random_id_generator,
            const std::vector<webrtc::Codec>& codecs,
            utils::SafeThread& worker_thread,
            utils::SafeThread& network_thread,
            std::weak_ptr<RemoteVideoSink> remote_video_sink,
            const std::map<int32_t, FrameTransformer::PayloadType>& payload_type_mapping,
            E2EEncryptor* encryptor
        );

        ~IncomingVideoChannel();

        [[nodiscard]] uint32_t ssrc() const;
    };

} // wrtc::interfaces::media::channels
