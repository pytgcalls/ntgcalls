//
// Created by Laky64 on 15/04/25.
//

#include <api/video/i420_buffer.h>
#ifdef IS_MACOS
#include <wrtc/interfaces/mtproto/extract_cv_pixel_buffer.hpp>
#endif
#include <rtc_base/logging.h>
#include <wrtc/interfaces/mtproto/video_streaming_part_internal.hpp>

namespace wrtc {

    VideoStreamingPartInternal::VideoStreamingPartInternal(
        std::string endpointId,
        const webrtc::VideoRotation rotation,
        bytes::binary&& fileData,
        const std::string& container
    ): endpointId(std::move(endpointId)), rotation(rotation) {
        frame = std::make_unique<VideoStreamingAVFrame>();
        avIoContext = std::make_unique<AVIOContextImpl>(std::move(fileData));
        const AVInputFormat *inputFormat = av_find_input_format(container.c_str());
        if (!inputFormat) {
            didReadToEnd = true;
            return;
        }

        inputFormatContext = avformat_alloc_context();
        if (!inputFormatContext) {
            didReadToEnd = true;
            return;
        }

        inputFormatContext->pb = avIoContext->getContext();

        if (avformat_open_input(&inputFormatContext, "", inputFormat, nullptr) < 0) {
            didReadToEnd = true;
            return;
        }

        if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
            didReadToEnd = true;

            avformat_close_input(&inputFormatContext);
            inputFormatContext = nullptr;
            return;
        }

        const AVCodecParameters *videoCodecParameters = nullptr;
        AVStream *videoStream = nullptr;
        for (int i = 0; i < inputFormatContext->nb_streams; i++) {
            AVStream *inStream = inputFormatContext->streams[i];

            const AVCodecParameters *inCodecpar = inStream->codecpar;
            if (inCodecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
                continue;
            }
            videoCodecParameters = inCodecpar;
            videoStream = inStream;
            break;
        }

        if (videoCodecParameters) {
            codecParameters = avcodec_parameters_alloc();
            avcodec_parameters_copy(codecParameters, videoCodecParameters);
            stream = videoStream;
        }
    }

    VideoStreamingPartInternal::~VideoStreamingPartInternal() {
        if (codecParameters) {
            avcodec_parameters_free(&codecParameters);
        }
        if (inputFormatContext) {
            avformat_close_input(&inputFormatContext);
        }
    }

    std::string VideoStreamingPartInternal::getEndpointId() const {
        return endpointId;
    }

    std::optional<std::unique_ptr<MediaDataPacket>> VideoStreamingPartInternal::readPacket() const {
        if (!inputFormatContext) {
            return std::nullopt;
        }
        auto packet = std::make_unique<MediaDataPacket>();
        if (av_read_frame(inputFormatContext, packet->getPacket()) < 0) {
            return std::nullopt;
        }
        return std::move(packet);
    }

    std::unique_ptr<DecodableFrame> VideoStreamingPartInternal::readNextDecodableFrame() const {
        while (true) {
            if (auto packet = readPacket(); packet.has_value()) {
                if (packet.value()->getPacket()->stream_index == stream->index) {
                    return std::make_unique<DecodableFrame>(
                        std::move(packet.value()),
                        packet.value()->getPacket()->pts,
                        packet.value()->getPacket()->dts
                    );
                }
            } else {
                return nullptr;
            }
        }
    }

    std::optional<VideoStreamingPartFrame> VideoStreamingPartInternal::convertCurrentFrame() {
#ifdef IS_MACOS
        if (const auto rtcFrame = frame->getFrame(); rtcFrame && rtcFrame->format == AV_PIX_FMT_VIDEOTOOLBOX && rtcFrame->data[3]) {
            const auto nativeFrame = extractCVPixelBuffer(rtcFrame->data[3]);
            const auto videoFrame = webrtc::VideoFrame::Builder()
                .set_video_frame_buffer(nativeFrame)
                .set_rotation(rotation)
                .build();
            return VideoStreamingPartFrame(endpointId, videoFrame, frame->pts(stream, firstFramePts), frameIndex);
        }
#endif
        const webrtc::scoped_refptr<webrtc::I420Buffer> i420Buffer = webrtc::I420Buffer::Copy(
            frame->getFrame()->width,
            frame->getFrame()->height,
            frame->getFrame()->data[0],
            frame->getFrame()->linesize[0],
            frame->getFrame()->data[1],
            frame->getFrame()->linesize[1],
            frame->getFrame()->data[2],
            frame->getFrame()->linesize[2]
        );
        if (i420Buffer) {
            const auto videoFrame = webrtc::VideoFrame::Builder()
                .set_video_frame_buffer(i420Buffer)
                .set_rotation(rotation)
                .build();
            return VideoStreamingPartFrame(endpointId, videoFrame, frame->pts(stream, firstFramePts), frameIndex);
        }
        return std::nullopt;
    }

    std::optional<VideoStreamingPartFrame> VideoStreamingPartInternal::getNextFrame(VideoStreamingSharedState* sharedState) {
        if (!stream) {
            return {};
        }
        if (!codecParameters) {
            return {};
        }
        sharedState->updateDecoderState(codecParameters, stream->time_base);

        while (true) {
            if (didReadToEnd) {
                if (!finalFrames.empty()) {
                    auto frame = finalFrames[0];
                    finalFrames.erase(finalFrames.begin());
                    return frame;
                }
                break;
            }
            if (const auto nextFrame = readNextDecodableFrame()) {
                if (const int sendStatus = sharedState->sendFrame(nextFrame.get()); sendStatus == 0) {
                    if (const int receiveStatus = sharedState->receiveFrame(frame.get()); receiveStatus == 0) {
                        if (auto convertedFrame = convertCurrentFrame()) {
                            frameIndex++;
                            return convertedFrame;
                        }
                    } else if (receiveStatus != AVERROR(EAGAIN)) {
                        RTC_LOG(LS_ERROR) << "avcodec_receive_frame failed with result: " << receiveStatus;
                        didReadToEnd = true;
                        break;
                    }
                } else {
                    RTC_LOG(LS_ERROR) << "avcodec_send_packet failed with result: " << sendStatus;
                    didReadToEnd = true;
                    return {};
                }
            } else {
                didReadToEnd = true;
                if (int sendStatus = sharedState->sendFrame(nullptr); sendStatus == 0) {
                    while (true) {
                        if (int receiveStatus = sharedState->receiveFrame(frame.get()); receiveStatus == 0) {
                            if (auto convertedFrame = convertCurrentFrame()) {
                                frameIndex++;
                                finalFrames.push_back(convertedFrame.value());
                            }
                        } else {
                            if (receiveStatus != AVERROR_EOF) {
                                RTC_LOG(LS_ERROR) << "avcodec_receive_frame (drain) failed with result: " << receiveStatus;
                            }
                            break;
                        }
                    }
                } else {
                    RTC_LOG(LS_ERROR) << "avcodec_send_packet (drain) failed with result: " << sendStatus;
                }
                sharedState->reset();
            }
        }
        return {};
    }
} // wrtc