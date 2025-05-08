//
// Created by Laky64 on 15/04/25.
//

#pragma once
#include <string>
#include <api/video/video_rotation.h>
#include <wrtc/models/video_streaming_av_frame.hpp>
#include <wrtc/models/video_streaming_part_frame.hpp>
#include <wrtc/interfaces/mtproto/avio_context_impl.hpp>
#include <wrtc/interfaces/mtproto/video_streaming_shared_state.hpp>
#include <wrtc/models/decodable_frame.hpp>

namespace wrtc {

    class VideoStreamingPartInternal {
        int frameIndex = 0;
        std::string endpointId;
        bool didReadToEnd = false;
        AVStream *stream = nullptr;
        double firstFramePts = -1.0;
        std::unique_ptr<VideoStreamingAVFrame> frame;
        std::unique_ptr<AVIOContextImpl> avIoContext;
        AVFormatContext *inputFormatContext = nullptr;
        AVCodecParameters *codecParameters = nullptr;
        std::vector<VideoStreamingPartFrame> finalFrames;
        webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0;

        std::optional<std::unique_ptr<MediaDataPacket>> readPacket() const;

        std::unique_ptr<DecodableFrame> readNextDecodableFrame() const;

        std::optional<VideoStreamingPartFrame> convertCurrentFrame();

    public:
        VideoStreamingPartInternal(
            std::string endpointId,
            webrtc::VideoRotation rotation,
            bytes::binary &&fileData,
            const std::string& container
        );

        ~VideoStreamingPartInternal();

        std::string getEndpointId() const;

        std::optional<VideoStreamingPartFrame> getNextFrame(VideoStreamingSharedState* sharedState);
    };

} // wrtc
