//
// Created by Lauren on 04/11/24.
//

#ifndef IS_ANDROID
#include <utility>
#include <libyuv/scale.h>
#include <wels/codec_ver.h>
#include <rtc_base/logging.h>
#include <api/video/video_bitrate_allocator.h>
#include <common_video/libyuv/include/webrtc_libyuv.h>
#include <modules/video_coding/include/video_codec_interface.h>
#include <system_wrappers/include/metrics.h>
#include <modules/video_coding/include/video_error_codes.h>
#include <modules/video_coding/utility/simulcast_utility.h>
#include <wrtc/video_factory/software/openh264/h264_encoder.hpp>
#include <modules/video_coding/utility/simulcast_rate_allocator.h>
#include <modules/video_coding/svc/create_scalability_structure.h>
#include <wrtc/utils/binary.hpp>

// Currently updated with the latest commit from
// https://webrtc.googlesource.com/src/+/c382c845754eda4a2c193102dc4c314d7dc4439f

namespace openh264 {
    H264Encoder::H264Encoder(webrtc::Environment env):
    has_reported_error_(false),
    has_reported_init_(false),
    max_payload_size_(0),
    number_of_cores_(0),
    env_(std::move(env)),
    packetization_mode_(webrtc::H264PacketizationMode::NonInterleaved),
    encoded_image_callback_(nullptr) {
        downscaled_buffers_.reserve(webrtc::kMaxSimulcastStreams - 1);
        encoded_images_.reserve(webrtc::kMaxSimulcastStreams);
        encoders_.reserve(webrtc::kMaxSimulcastStreams);
        configurations_.reserve(webrtc::kMaxSimulcastStreams);
        tl0sync_limit_.reserve(webrtc::kMaxSimulcastStreams);
        svc_controllers_.reserve(webrtc::kMaxSimulcastStreams);
    }

    H264Encoder::~H264Encoder() {
        Release();
    }

    int32_t H264Encoder::InitEncode(const webrtc::VideoCodec* inst, const Settings& settings) {
        report_init();
        if (!inst || inst->codecType != webrtc::kVideoCodecH264) {
            report_error();
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }
        if (inst->maxFramerate == 0) {
            report_error();
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }
        if (inst->width < 1 || inst->height < 1) {
            report_error();
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }

        if (const int32_t release_ret = Release(); release_ret != WEBRTC_VIDEO_CODEC_OK) {
          report_error();
          return release_ret;
        }

        const int number_of_streams = webrtc::SimulcastUtility::NumberOfSimulcastStreams(*inst);
        if (const bool doing_simulcast = number_of_streams > 1; doing_simulcast && !webrtc::SimulcastUtility::ValidSimulcastParameters(*inst, number_of_streams)) {
          return WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED;
        }
        downscaled_buffers_.resize(number_of_streams - 1);
        encoded_images_.resize(number_of_streams);
        encoders_.resize(number_of_streams);
        pictures_.resize(number_of_streams);
        svc_controllers_.resize(number_of_streams);
        scalability_modes_.resize(number_of_streams);
        configurations_.resize(number_of_streams);
        tl0sync_limit_.resize(number_of_streams);
        max_payload_size_ = settings.max_payload_size;
        number_of_cores_ = settings.number_of_cores;
        encoder_thread_limit_ = settings.encoder_thread_limit;
        codec_ = *inst;
        if (codec_.numberOfSimulcastStreams == 0) {
          codec_.simulcastStream[0].width = codec_.width;
          codec_.simulcastStream[0].height = codec_.height;
        }

        for (int i = 0, idx = number_of_streams - 1; i < number_of_streams; ++i, --idx) {
            ISVCEncoder* openh264_encoder;
            if (WelsCreateSVCEncoder(&openh264_encoder) != 0) {
                RTC_LOG(LS_ERROR) << "Failed to create OpenH264 encoder";
                RTC_DCHECK(!openh264_encoder);
                Release();
                report_error();
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
            RTC_DCHECK(openh264_encoder);
            int32_t i_trace_level = WELS_LOG_QUIET;
            openh264_encoder->SetOption(ENCODER_OPTION_TRACE_LEVEL, &i_trace_level);
            encoders_[i] = openh264_encoder;

            configurations_[i].simulcast_idx = idx;
            configurations_[i].sending = false;
            configurations_[i].width = codec_.simulcastStream[idx].width;
            configurations_[i].height = codec_.simulcastStream[idx].height;
            configurations_[i].max_frame_rate = static_cast<float>(codec_.maxFramerate);
            configurations_[i].frame_dropping_on = codec_.GetFrameDropEnabled();
            configurations_[i].key_frame_interval = codec_.H264()->keyFrameInterval;
            configurations_[i].num_temporal_layers = std::max(codec_.H264()->numberOfTemporalLayers, codec_.simulcastStream[idx].numberOfTemporalLayers);
            if (i > 0) {
                downscaled_buffers_[i - 1] = webrtc::I420Buffer::Create(
                    configurations_[i].width, configurations_[i].height,
                    configurations_[i].width, configurations_[i].width / 2,
                    configurations_[i].width / 2
                );
            }
            configurations_[i].max_bps = codec_.maxBitrate * 1000;
            configurations_[i].target_bps = codec_.startBitrate * 1000;

            const SEncParamExt encoder_params = create_encoder_params(i);

            if (openh264_encoder->InitializeExt(&encoder_params) != 0) {
                RTC_LOG(LS_ERROR) << "Failed to initialize OpenH264 encoder";
                Release();
                report_error();
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
            int video_format = videoFormatI420;
            openh264_encoder->SetOption(ENCODER_OPTION_DATAFORMAT, &video_format);
            const size_t new_capacity = CalcBufferSize(
                webrtc::VideoType::kI420,
                codec_.simulcastStream[idx].width,
                codec_.simulcastStream[idx].height
            );
            encoded_images_[i].SetEncodedData(webrtc::EncodedImageBuffer::Create(new_capacity));
            encoded_images_[i]._encodedWidth = codec_.simulcastStream[idx].width;
            encoded_images_[i]._encodedHeight = codec_.simulcastStream[idx].height;
            encoded_images_[i].set_size(0);

            tl0sync_limit_[i] = configurations_[i].num_temporal_layers;
            scalability_modes_[i] = scalability_mode_from_temporal_layers(configurations_[i].num_temporal_layers);
            if (scalability_modes_[i].has_value()) {
                svc_controllers_[i] = CreateScalabilityStructure(*scalability_modes_[i]);
                if (svc_controllers_[i] == nullptr) {
                  RTC_LOG(LS_ERROR) << "Failed to create scalability structure";
                  Release();
                  report_error();
                  return WEBRTC_VIDEO_CODEC_ERROR;
                }
            }
        }

        webrtc::SimulcastRateAllocator init_allocator(env_, codec_);
        const webrtc::VideoBitrateAllocation allocation = init_allocator.Allocate(
            webrtc::VideoBitrateAllocationParameters(
                webrtc::DataRate::KilobitsPerSec(codec_.startBitrate),
                codec_.maxFramerate
            )
        );
        SetRates(RateControlParameters(allocation, codec_.maxFramerate));
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t H264Encoder::RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) {
        encoded_image_callback_ = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t H264Encoder::Release() {
        while (!encoders_.empty()) {
            if (ISVCEncoder* openh264_encoder = encoders_.back()) {
                RTC_CHECK_EQ(0, openh264_encoder->Uninitialize());
                WelsDestroySVCEncoder(openh264_encoder);
            }
            encoders_.pop_back();
        }
        downscaled_buffers_.clear();
        configurations_.clear();
        encoded_images_.clear();
        pictures_.clear();
        tl0sync_limit_.clear();
        svc_controllers_.clear();
        scalability_modes_.clear();
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t H264Encoder::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frame_types) {
        if (encoders_.empty()) {
            report_error();
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        if (!encoded_image_callback_) {
            RTC_LOG(LS_VERBOSE)
                << "InitEncode() has been called, but a callback function "
                   "has not been set with RegisterEncodeCompleteCallback()";
            report_error();
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        const webrtc::scoped_refptr<webrtc::I420BufferInterface> frame_buffer = frame.video_frame_buffer()->ToI420();
        if (!frame_buffer) {
            RTC_LOG(LS_ERROR) << "Failed to convert "
                              << VideoFrameBufferTypeToString(frame.video_frame_buffer()->type())
                              << " image to I420. Can't encode frame.";
            return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
        }

        RTC_CHECK(frame_buffer->type() == webrtc::VideoFrameBuffer::Type::kI420 || frame_buffer->type() == webrtc::VideoFrameBuffer::Type::kI420A);

        bool is_keyframe_needed = false;
        for (auto & [simulcastIdx, width, height, sending, keyFrameRequest, maxFrameRate, targetBps, maxBps, frameDroppingOn, keyFrameInterval, numTemporalLayers] : configurations_) {
            if (keyFrameRequest && sending) {
                is_keyframe_needed = true;
                break;
            }
        }

        RTC_DCHECK_EQ(configurations_[0].width, frame_buffer->width());
        RTC_DCHECK_EQ(configurations_[0].height, frame_buffer->height());

        for (size_t i = 0; i < encoders_.size(); ++i) {
            pictures_[i] = {};
            pictures_[i].iPicWidth = configurations_[i].width;
            pictures_[i].iPicHeight = configurations_[i].height;
            pictures_[i].iColorFormat = videoFormatI420;
            pictures_[i].uiTimeStamp = frame.ntp_time_ms();

            if (i == 0) {
                pictures_[i].iStride[0] = frame_buffer->StrideY();
                pictures_[i].iStride[1] = frame_buffer->StrideU();
                pictures_[i].iStride[2] = frame_buffer->StrideV();
                pictures_[i].pData[0] = const_cast<bytes::byte*>(frame_buffer->DataY());
                pictures_[i].pData[1] = const_cast<bytes::byte*>(frame_buffer->DataU());
                pictures_[i].pData[2] = const_cast<bytes::byte*>(frame_buffer->DataV());
            } else {
                pictures_[i].iStride[0] = downscaled_buffers_[i - 1]->StrideY();
                pictures_[i].iStride[1] = downscaled_buffers_[i - 1]->StrideU();
                pictures_[i].iStride[2] = downscaled_buffers_[i - 1]->StrideV();
                pictures_[i].pData[0] = const_cast<bytes::byte*>(downscaled_buffers_[i - 1]->DataY());
                pictures_[i].pData[1] = const_cast<bytes::byte*>(downscaled_buffers_[i - 1]->DataU());
                pictures_[i].pData[2] = const_cast<bytes::byte*>(downscaled_buffers_[i - 1]->DataV());
                I420Scale(
                    pictures_[i - 1].pData[0], pictures_[i - 1].iStride[0],
                    pictures_[i - 1].pData[1], pictures_[i - 1].iStride[1],
                    pictures_[i - 1].pData[2], pictures_[i - 1].iStride[2],
                    configurations_[i - 1].width,
                    configurations_[i - 1].height,
                    pictures_[i].pData[0], pictures_[i].iStride[0],
                    pictures_[i].pData[1], pictures_[i].iStride[1],
                    pictures_[i].pData[2], pictures_[i].iStride[2],
                    configurations_[i].width,
                    configurations_[i].height,
                    libyuv::kFilterBox
                );
            }

            if (!configurations_[i].sending) {
                continue;
            }
            if (frame_types != nullptr && i < frame_types->size()) {
                if ((*frame_types)[i] == webrtc::VideoFrameType::kEmptyFrame) {
                    continue;
                }
            }

            const auto simulcast_idx = static_cast<size_t>(configurations_[i].simulcast_idx);
            const bool send_key_frame = is_keyframe_needed || (frame_types && simulcast_idx < frame_types->size() &&  (*frame_types)[simulcast_idx] == webrtc::VideoFrameType::kVideoFrameKey);
            if (send_key_frame) {
                encoders_[i]->ForceIntraFrame(true);
                configurations_[i].key_frame_request = false;
            }

            SFrameBSInfo info = {};

            std::vector<webrtc::ScalableVideoController::LayerFrameConfig> layer_frames;
            if (svc_controllers_[i]) {
                layer_frames = svc_controllers_[i]->NextFrameConfig(send_key_frame);
                RTC_CHECK_EQ(layer_frames.size(), 1);
            }

            if (const int enc_ret = encoders_[i]->EncodeFrame(&pictures_[i], &info); enc_ret != 0) {
                RTC_LOG(LS_ERROR)
                    << "OpenH264 frame encoding failed, EncodeFrame returned " << enc_ret
                    << ".";
                report_error();
                return WEBRTC_VIDEO_CODEC_ERROR;
            }

            encoded_images_[i]._encodedWidth = configurations_[i].width;
            encoded_images_[i]._encodedHeight = configurations_[i].height;
            encoded_images_[i].SetRtpTimestamp(frame.rtp_timestamp());
            encoded_images_[i].SetColorSpace(frame.color_space());
            encoded_images_[i]._frameType = convert_to_video_frame_type(info.eFrameType);
            encoded_images_[i].SetSimulcastIndex(configurations_[i].simulcast_idx);

            rtp_fragmentize(&encoded_images_[i], &info);

            if (encoded_images_[i].size() > 0) {
                h264_bitstream_parser_.ParseBitstream(encoded_images_[i]);
                encoded_images_[i].qp_ = h264_bitstream_parser_.GetLastSliceQp().value_or(-1);
                webrtc::CodecSpecificInfo codec_specific;
                codec_specific.codecType = webrtc::kVideoCodecH264;
                codec_specific.codecSpecific.H264.packetization_mode = packetization_mode_;
                codec_specific.codecSpecific.H264.temporal_idx = webrtc::kNoTemporalIdx;
                codec_specific.codecSpecific.H264.idr_frame = info.eFrameType == videoFrameTypeIDR;
                codec_specific.codecSpecific.H264.base_layer_sync = false;
                if (configurations_[i].num_temporal_layers > 1) {
                    const uint8_t tid = info.sLayerInfo[0].uiTemporalId;
                    codec_specific.codecSpecific.H264.temporal_idx = tid;
                    codec_specific.codecSpecific.H264.base_layer_sync = tid > 0 && tid < tl0sync_limit_[i];
                    if (svc_controllers_[i]) {
                        if (encoded_images_[i]._frameType == webrtc::VideoFrameType::kVideoFrameKey) {
                            layer_frames = svc_controllers_[i]->NextFrameConfig(true);
                            RTC_CHECK_EQ(layer_frames.size(), 1);
                            RTC_DCHECK_EQ(layer_frames[0].TemporalId(), 0);
                            RTC_DCHECK_EQ(layer_frames[0].IsKeyframe(), true);
                        }
                        if (layer_frames[0].TemporalId() != tid) {
                            RTC_LOG(LS_VERBOSE)
                                << "Encoder produced a frame with temporal id " << tid
                                << ", expected " << layer_frames[0].TemporalId() << ".";
                            continue;
                        }
                        encoded_images_[i].SetTemporalIndex(tid);
                    }
                    if (codec_specific.codecSpecific.H264.base_layer_sync) {
                        tl0sync_limit_[i] = tid;
                    }
                    if (tid == 0) {
                        tl0sync_limit_[i] = configurations_[i].num_temporal_layers;
                    }
                }
                if (svc_controllers_[i]) {
                    codec_specific.generic_frame_info = svc_controllers_[i]->OnEncodeDone(layer_frames[0]);
                    if (encoded_images_[i]._frameType == webrtc::VideoFrameType::kVideoFrameKey && codec_specific.generic_frame_info.has_value()) {
                        codec_specific.template_structure = svc_controllers_[i]->DependencyStructure();
                    }
                    codec_specific.scalability_mode = scalability_modes_[i];
                }
                encoded_image_callback_->OnEncodedImage(encoded_images_[i], &codec_specific);
            }
        }
        return WEBRTC_VIDEO_CODEC_OK;
    }

    void H264Encoder::SetRates(const RateControlParameters& parameters) {
        if (encoders_.empty()) {
            RTC_LOG(LS_VERBOSE) << "SetRates() while uninitialized.";
            return;
        }

        if (parameters.framerate_fps < 1.0) {
            RTC_LOG(LS_VERBOSE) << "Invalid frame rate: " << parameters.framerate_fps;
            return;
        }

        if (parameters.bitrate.get_sum_bps() == 0) {
            for (auto & configuration : configurations_) {
                configuration.set_stream_state(false);
            }
            return;
        }

        codec_.maxFramerate = static_cast<uint32_t>(parameters.framerate_fps);

        size_t stream_idx = encoders_.size() - 1;
        for (size_t i = 0; i < encoders_.size(); ++i, --stream_idx) {
            configurations_[i].target_bps = parameters.bitrate.GetSpatialLayerSum(stream_idx);
            configurations_[i].max_frame_rate = static_cast<float>(parameters.framerate_fps);

            if (configurations_[i].target_bps) {
                configurations_[i].set_stream_state(true);

                // Update h264 encoder.
                SBitrateInfo target_bitrate = {};
                target_bitrate.iLayer = SPATIAL_LAYER_ALL,
                target_bitrate.iBitrate = static_cast<int>(configurations_[i].target_bps);
                encoders_[i]->SetOption(ENCODER_OPTION_BITRATE, &target_bitrate);
                encoders_[i]->SetOption(ENCODER_OPTION_FRAME_RATE, &configurations_[i].max_frame_rate);
            } else {
                configurations_[i].set_stream_state(false);
            }
        }
    }

    webrtc::VideoEncoder::EncoderInfo H264Encoder::GetEncoderInfo() const {
        EncoderInfo info;
        info.supports_native_handle = false;
        info.implementation_name = "OpenH264";
        info.scaling_settings = ScalingSettings(kLowH264QpThreshold, kHighH264QpThreshold);
        info.is_hardware_accelerated = false;
        info.supports_simulcast = true;
        info.preferred_pixel_formats = {webrtc::VideoFrameBuffer::Type::kI420};
        return info;
    }

    void H264Encoder::report_error() {
        if (has_reported_error_) return;
        RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.H264EncoderImpl.Event", H264EncoderEventError, H264EncoderEventMax);
        has_reported_error_ = true;
    }

    webrtc::VideoFrameType H264Encoder::convert_to_video_frame_type(const EVideoFrameType type) {
        switch (type) {
        case videoFrameTypeIDR:
            return webrtc::VideoFrameType::kVideoFrameKey;
        case videoFrameTypeSkip:
        case videoFrameTypeI:
        case videoFrameTypeP:
        case videoFrameTypeIPMixed:
            return webrtc::VideoFrameType::kVideoFrameDelta;
        case videoFrameTypeInvalid:
            break;
        }
        RTC_DCHECK_NOTREACHED() << "Unexpected/invalid frame type: " << type;
        return webrtc::VideoFrameType::kEmptyFrame;
    }

    void H264Encoder::rtp_fragmentize(webrtc::EncodedImage* encoded_image, SFrameBSInfo* info) {
        size_t required_capacity = 0;
        size_t fragments_count = 0;
        for (int layer = 0; layer < info->iLayerNum; ++layer) {
            const auto& [uiTemporalId, uiSpatialId, uiQualityId, eFrameType, uiLayerType, iSubSeqId, iNalCount, pNalLengthInByte, pBsBuf, rPsnr] = info->sLayerInfo[layer];
            for (int nal = 0; nal < iNalCount; ++nal, ++fragments_count) {
                RTC_CHECK_GE(pNalLengthInByte[nal], 0);
                RTC_CHECK_LE(pNalLengthInByte[nal], std::numeric_limits<size_t>::max() - required_capacity);
                required_capacity += pNalLengthInByte[nal];
            }
        }
        const auto buffer = webrtc::EncodedImageBuffer::Create(required_capacity);
        encoded_image->SetEncodedData(buffer);
        size_t frag = 0;
        encoded_image->set_size(0);
        for (int layer = 0; layer < info->iLayerNum; ++layer) {
            const auto& [uiTemporalId, uiSpatialId, uiQualityId, eFrameType, uiLayerType, iSubSeqId, iNalCount, pNalLengthInByte, pBsBuf, rPsnr] = info->sLayerInfo[layer];
            size_t layer_len = 0;
            for (int nal = 0; nal < iNalCount; ++nal, ++frag) {
                constexpr uint8_t kStartCode[4] = {0, 0, 0, 1};
                RTC_DCHECK_GE(pNalLengthInByte[nal], 4);
                RTC_DCHECK_EQ(pBsBuf[layer_len + 0], kStartCode[0]);
                RTC_DCHECK_EQ(pBsBuf[layer_len + 1], kStartCode[1]);
                RTC_DCHECK_EQ(pBsBuf[layer_len + 2], kStartCode[2]);
                RTC_DCHECK_EQ(pBsBuf[layer_len + 3], kStartCode[3]);
                layer_len += pNalLengthInByte[nal];
            }
            std::memcpy(buffer->data() + encoded_image->size(), pBsBuf, layer_len);
            encoded_image->set_size(encoded_image->size() + layer_len);
        }
    }

    void H264Encoder::report_init() {
        if (has_reported_init_)
            return;
        RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.H264EncoderImpl.Event", H264EncoderEventInit, H264EncoderEventMax);
        has_reported_init_ = true;
    }

    std::optional<webrtc::ScalabilityMode> H264Encoder::scalability_mode_from_temporal_layers(const int num_temporal_layers) {
        switch (num_temporal_layers) {
        case 0:
            break;
        case 1:
            return webrtc::ScalabilityMode::kL1T1;
        case 2:
            return webrtc::ScalabilityMode::kL1T2;
        case 3:
            return webrtc::ScalabilityMode::kL1T3;
        default:
            RTC_DCHECK_NOTREACHED();
        }
        return std::nullopt;
    }

    SEncParamExt H264Encoder::create_encoder_params(const size_t i) const {
        SEncParamExt encoder_params;
        encoders_[i]->GetDefaultParams(&encoder_params);
        if (codec_.mode == webrtc::VideoCodecMode::kRealtimeVideo) {
            encoder_params.iUsageType = CAMERA_VIDEO_REAL_TIME;
        } else if (codec_.mode == webrtc::VideoCodecMode::kScreensharing) {
            encoder_params.iUsageType = SCREEN_CONTENT_REAL_TIME;
        } else {
            RTC_DCHECK_NOTREACHED();
        }
        encoder_params.iPicWidth = configurations_[i].width;
        encoder_params.iPicHeight = configurations_[i].height;
        encoder_params.iTargetBitrate = static_cast<int>(configurations_[i].target_bps);
        encoder_params.iMaxBitrate = UNSPECIFIED_BIT_RATE;
        encoder_params.iRCMode = RC_BITRATE_MODE;
        encoder_params.fMaxFrameRate = configurations_[i].max_frame_rate;
        encoder_params.bEnableFrameSkip = configurations_[i].frame_dropping_on;
        encoder_params.uiIntraPeriod = configurations_[i].key_frame_interval;
        encoder_params.eSpsPpsIdStrategy = SPS_LISTING;
        encoder_params.uiMaxNalSize = 0;
        encoder_params.iMultipleThreadIdc = number_of_threads(
            encoder_thread_limit_,
            encoder_params.iPicWidth,
            encoder_params.iPicHeight,
            number_of_cores_
        );
        encoder_params.sSpatialLayers[0].iVideoWidth = encoder_params.iPicWidth;
        encoder_params.sSpatialLayers[0].iVideoHeight = encoder_params.iPicHeight;
        encoder_params.sSpatialLayers[0].fFrameRate = encoder_params.fMaxFrameRate;
        encoder_params.sSpatialLayers[0].iSpatialBitrate = encoder_params.iTargetBitrate;
        encoder_params.sSpatialLayers[0].iMaxSpatialBitrate = encoder_params.iMaxBitrate;
        encoder_params.iTemporalLayerNum = configurations_[i].num_temporal_layers;
        if (encoder_params.iTemporalLayerNum > 1) {
            encoder_params.iNumRefFrame = encoder_params.iTemporalLayerNum - 1;
        }
        RTC_LOG(LS_VERBOSE) << "OpenH264 version is " << OPENH264_MAJOR << "." << OPENH264_MINOR;
        switch (packetization_mode_) {
        case webrtc::H264PacketizationMode::SingleNalUnit:
            encoder_params.sSpatialLayers[0].sSliceArgument.uiSliceNum = 1;
            encoder_params.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SIZELIMITED_SLICE;
            encoder_params.sSpatialLayers[0].sSliceArgument.uiSliceSizeConstraint = static_cast<unsigned int>(max_payload_size_);
            RTC_LOG(LS_VERBOSE) << "Encoder is configured with NALU constraint: " << max_payload_size_ << " bytes";
            break;
        case webrtc::H264PacketizationMode::NonInterleaved:
            encoder_params.sSpatialLayers[0].sSliceArgument.uiSliceNum = 1;
            encoder_params.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
            break;
        }
        return encoder_params;
    }

    int H264Encoder::number_of_threads(const std::optional<int> encoder_thread_limit, const int width, const int height, const int number_of_cores) {
        if (encoder_thread_limit.has_value()) {
            const int limit = encoder_thread_limit.value();
            RTC_DCHECK_GE(limit, 1);
            if (width * height >= 1920 * 1080 && number_of_cores > 8) {
                return std::min(limit, 8);
            }
            if (width * height > 1280 * 960 && number_of_cores >= 6) {
                return std::min(limit, 3);
            }
            if (width * height > 640 * 480 && number_of_cores >= 3) {
                return std::min(limit, 2);
            }
            return 1;
        }
        return 1;
    }
} // openh264
#endif