//
// Created by Lauren on 15/04/25.
//

#pragma once
#include <string>
#include <api/video/video_rotation.h>
#include <wrtc/interfaces/mtproto/avio_context_impl.hpp>
#include <wrtc/interfaces/mtproto/video_streaming_shared_state.hpp>
#include <wrtc/interfaces/media/decodable_frame.hpp>
#include <wrtc/interfaces/media/video_streaming_av_frame.hpp>
#include <wrtc/interfaces/media/video_streaming_part_frame.hpp>

namespace wrtc::interfaces::mtproto {

    class VideoStreamingPartInternal {
        int frame_index_ = 0;
        std::string endpoint_id_;
        bool did_read_to_end_ = false;
        AVStream* stream_ = nullptr;
        double first_frame_pts_ = -1.0;
        std::unique_ptr<media::VideoStreamingAVFrame> frame_;
        std::unique_ptr<AVIOContextImpl> av_io_context_;
        AVFormatContext* input_format_context_ = nullptr;
        AVCodecParameters* codec_parameters_ = nullptr;
        std::vector<media::VideoStreamingPartFrame> final_frames_;
        webrtc::VideoRotation rotation_ = webrtc::VideoRotation::kVideoRotation_0;

        [[nodiscard]] std::optional<std::unique_ptr<media::MediaDataPacket>> read_packet() const;

        [[nodiscard]] std::unique_ptr<media::DecodableFrame> read_next_decodable_frame() const;

        std::optional<media::VideoStreamingPartFrame> convert_current_frame();

    public:
        VideoStreamingPartInternal(
            std::string endpoint_id,
            webrtc::VideoRotation rotation,
            bytes::binary&& file_data,
            const std::string& container
        );

        ~VideoStreamingPartInternal();

        [[nodiscard]] std::string get_endpoint_id() const;

        std::optional<media::VideoStreamingPartFrame> get_next_frame(VideoStreamingSharedState* shared_state);
    };

} // wrtc::interfaces::mtproto
