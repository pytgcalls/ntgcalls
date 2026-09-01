//
// Created by Lauren on 15/04/25.
//

#include <api/video/i420_buffer.h>
#ifdef IS_MACOS
#include <wrtc/interfaces/mtproto/extract_cv_pixel_buffer.hpp>
#endif
#include <rtc_base/logging.h>
#include <wrtc/interfaces/mtproto/video_streaming_part_internal.hpp>

namespace wrtc::interfaces::mtproto {

    VideoStreamingPartInternal::VideoStreamingPartInternal(
        std::string endpoint_id,
        const webrtc::VideoRotation rotation,
        bytes::binary&& file_data,
        const std::string& container
    ): endpoint_id_(std::move(endpoint_id)), rotation_(rotation) {
        frame_ = std::make_unique<media::VideoStreamingAVFrame>();
        av_io_context_ = std::make_unique<AVIOContextImpl>(std::move(file_data));
        const AVInputFormat* input_format = av_find_input_format(container.c_str());
        if (!input_format) {
            did_read_to_end_ = true;
            return;
        }

        input_format_context_ = avformat_alloc_context();
        if (!input_format_context_) {
            did_read_to_end_ = true;
            return;
        }

        input_format_context_->pb = av_io_context_->get_context();

        if (avformat_open_input(&input_format_context_, "", input_format, nullptr) < 0) {
            avformat_free_context(input_format_context_);
            input_format_context_ = nullptr;
            did_read_to_end_ = true;
            return;
        }

        if (avformat_find_stream_info(input_format_context_, nullptr) < 0) {
            did_read_to_end_ = true;

            avformat_close_input(&input_format_context_);
            input_format_context_ = nullptr;
            return;
        }

        const AVCodecParameters* video_codec_parameters = nullptr;
        AVStream* video_stream = nullptr;
        for (int i = 0; i < input_format_context_->nb_streams; i++) {
            AVStream* in_stream = input_format_context_->streams[i];

            const AVCodecParameters* in_codecpar = in_stream->codecpar;
            if (in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
                continue;
            }
            video_codec_parameters = in_codecpar;
            video_stream = in_stream;
            break;
        }

        if (video_codec_parameters) {
            codec_parameters_ = avcodec_parameters_alloc();
            avcodec_parameters_copy(codec_parameters_, video_codec_parameters);
            stream_ = video_stream;
        }
    }

    VideoStreamingPartInternal::~VideoStreamingPartInternal() {
        if (codec_parameters_) {
            avcodec_parameters_free(&codec_parameters_);
        }
        if (input_format_context_) {
            avformat_close_input(&input_format_context_);
        }
    }

    std::string VideoStreamingPartInternal::get_endpoint_id() const {
        return endpoint_id_;
    }

    std::optional<std::unique_ptr<media::MediaDataPacket>> VideoStreamingPartInternal::read_packet() const {
        if (!input_format_context_) {
            return std::nullopt;
        }
        auto packet = std::make_unique<media::MediaDataPacket>();
        if (av_read_frame(input_format_context_, packet->get_packet()) < 0) {
            return std::nullopt;
        }
        return std::move(packet);
    }

    std::unique_ptr<media::DecodableFrame> VideoStreamingPartInternal::read_next_decodable_frame() const {
        while (true) {
            if (auto packet = read_packet(); packet.has_value()) {
                if (packet.value()->get_packet()->stream_index == stream_->index) {
                    return std::make_unique<media::DecodableFrame>(
                        std::move(packet.value()),
                        packet.value()->get_packet()->pts,
                        packet.value()->get_packet()->dts
                    );
                }
            } else {
                return nullptr;
            }
        }
    }

    std::optional<media::VideoStreamingPartFrame> VideoStreamingPartInternal::convert_current_frame() {
#ifdef IS_MACOS
        if (const auto rtc_frame = frame_->get_frame(); rtc_frame && rtc_frame->format == AV_PIX_FMT_VIDEOTOOLBOX && rtc_frame->data[3]) {
            const auto native_frame = extract_cv_pixel_buffer(rtc_frame->data[3]);
            const auto video_frame = webrtc::VideoFrame::Builder()
                                         .set_video_frame_buffer(native_frame)
                                         .set_rotation(rotation_)
                                         .build();
            return media::VideoStreamingPartFrame(endpoint_id_, video_frame, frame_->pts(stream_, first_frame_pts_), frame_index_);
        }
#endif
        const webrtc::scoped_refptr<webrtc::I420Buffer> i420_buffer = webrtc::I420Buffer::Copy(
            frame_->get_frame()->width,
            frame_->get_frame()->height,
            frame_->get_frame()->data[0],
            frame_->get_frame()->linesize[0],
            frame_->get_frame()->data[1],
            frame_->get_frame()->linesize[1],
            frame_->get_frame()->data[2],
            frame_->get_frame()->linesize[2]
        );
        if (i420_buffer) {
            const auto video_frame = webrtc::VideoFrame::Builder()
                                         .set_video_frame_buffer(i420_buffer)
                                         .set_rotation(rotation_)
                                         .build();
            return media::VideoStreamingPartFrame(endpoint_id_, video_frame, frame_->pts(stream_, first_frame_pts_), frame_index_);
        }
        return std::nullopt;
    }

    std::optional<media::VideoStreamingPartFrame> VideoStreamingPartInternal::get_next_frame(VideoStreamingSharedState* shared_state) {
        if (!stream_) {
            return {};
        }
        if (!codec_parameters_) {
            return {};
        }
        shared_state->update_decoder_state(codec_parameters_, stream_->time_base);

        while (true) {
            if (did_read_to_end_) {
                if (!final_frames_.empty()) {
                    auto frame = final_frames_[0];
                    final_frames_.erase(final_frames_.begin());
                    return frame;
                }
                break;
            }
            if (const auto next_frame = read_next_decodable_frame()) {
                if (const int send_status = shared_state->send_frame(next_frame.get()); send_status == 0) {
                    if (const int receive_status = shared_state->receive_frame(frame_.get()); receive_status == 0) {
                        if (auto converted_frame = convert_current_frame()) {
                            frame_index_++;
                            return converted_frame;
                        }
                    } else if (receive_status != AVERROR(EAGAIN)) {
                        RTC_LOG(LS_ERROR) << "avcodec_receive_frame failed with result: " << receive_status;
                        did_read_to_end_ = true;
                        break;
                    }
                } else {
                    RTC_LOG(LS_ERROR) << "avcodec_send_packet failed with result: " << send_status;
                    did_read_to_end_ = true;
                    return {};
                }
            } else {
                did_read_to_end_ = true;
                if (const int send_status = shared_state->send_frame(nullptr); send_status == 0) {
                    while (true) {
                        if (const int receive_status = shared_state->receive_frame(frame_.get()); receive_status == 0) {
                            if (auto converted_frame = convert_current_frame()) {
                                frame_index_++;
                                final_frames_.push_back(converted_frame.value());
                            }
                        } else {
                            if (receive_status != AVERROR_EOF) {
                                RTC_LOG(LS_ERROR) << "avcodec_receive_frame (drain) failed with result: " << receive_status;
                            }
                            break;
                        }
                    }
                } else {
                    RTC_LOG(LS_ERROR) << "avcodec_send_packet (drain) failed with result: " << send_status;
                }
                shared_state->reset();
            }
        }
        return {};
    }
} // wrtc::interfaces::mtproto
