//
// Created by Laky64 on 25/10/24.
//

#pragma once
#include <pc/channel.h>
#include <pc/rtp_transport.h>
#include <rtc_base/third_party/sigslot/sigslot.h>
#include <wrtc/interfaces/media/channel_manager.hpp>
#include <wrtc/interfaces/media/raw_video_sink.hpp>
#include <wrtc/interfaces/media/remote_video_sink.hpp>
#include <wrtc/models/media_content.hpp>

namespace wrtc {

    class IncomingVideoChannel final : public sigslot::has_slots<> {
        uint32_t _ssrc = 0;
        std::unique_ptr<cricket::VideoChannel> channel;
        std::unique_ptr<webrtc::VideoBitrateAllocatorFactory> videoBitrateAllocatorFactory;
        rtc::Thread* workerThread;
        rtc::Thread* networkThread;
        std::unique_ptr<RawVideoSink> sink;

    public:
        IncomingVideoChannel(
            webrtc::Call* call,
            ChannelManager *channelManager,
            webrtc::RtpTransport* rtpTransport,
            std::vector<SsrcGroup> ssrcGroups,
            rtc::UniqueRandomIdGenerator *randomIdGenerator,
            const std::vector<cricket::Codec>& codecs,
            rtc::Thread *workerThread,
            rtc::Thread* networkThread,
            std::weak_ptr<RemoteVideoSink> remoteVideoSink
        );

        ~IncomingVideoChannel() override;

        [[nodiscard]] uint32_t ssrc() const;
    };

} // wrtc
