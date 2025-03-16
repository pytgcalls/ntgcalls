//
// Created by Laky64 on 04/11/24.
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

namespace openh264 {
    H264Encoder::H264Encoder(webrtc::Environment env):
    hasReportedError(false),
    hasReportedInit(false),
    maxPayloadSize(0),
    numberOfCores(0),
    env(std::move(env)),
    packetizationMode(webrtc::H264PacketizationMode::NonInterleaved),
    encodedImageCallback(nullptr) {
        downscaledBuffers.reserve(webrtc::kMaxSimulcastStreams - 1);
        encodedImages.reserve(webrtc::kMaxSimulcastStreams);
        encoders.reserve(webrtc::kMaxSimulcastStreams);
        configurations.reserve(webrtc::kMaxSimulcastStreams);
        tl0syncLimit.reserve(webrtc::kMaxSimulcastStreams);
        svcControllers.reserve(webrtc::kMaxSimulcastStreams);
    }

    H264Encoder::~H264Encoder() {
        Release();
    }

    int32_t H264Encoder::InitEncode(const webrtc::VideoCodec* inst, const Settings& settings) {
        ReportInit();
        if (!inst || inst->codecType != webrtc::kVideoCodecH264) {
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }
        if (inst->maxFramerate == 0) {
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }
        if (inst->width < 1 || inst->height < 1) {
            ReportError();
            return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
        }

        if (const int32_t releaseRet = Release(); releaseRet != WEBRTC_VIDEO_CODEC_OK) {
          ReportError();
          return releaseRet;
        }

        const int numberOfStreams = webrtc::SimulcastUtility::NumberOfSimulcastStreams(*inst);
        if (const bool doingSimulcast = numberOfStreams > 1; doingSimulcast && !webrtc::SimulcastUtility::ValidSimulcastParameters(*inst, numberOfStreams)) {
          return WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED;
        }
        downscaledBuffers.resize(numberOfStreams - 1);
        encodedImages.resize(numberOfStreams);
        encoders.resize(numberOfStreams);
        pictures.resize(numberOfStreams);
        svcControllers.resize(numberOfStreams);
        scalabilityModes.resize(numberOfStreams);
        configurations.resize(numberOfStreams);
        tl0syncLimit.resize(numberOfStreams);
        maxPayloadSize = settings.max_payload_size;
        numberOfCores = settings.number_of_cores;
        encoderThreadLimit = settings.encoder_thread_limit;
        codec = *inst;
        if (codec.numberOfSimulcastStreams == 0) {
          codec.simulcastStream[0].width = codec.width;
          codec.simulcastStream[0].height = codec.height;
        }

        for (int i = 0, idx = numberOfStreams - 1; i < numberOfStreams; ++i, --idx) {
            ISVCEncoder* openh264Encoder;
            if (WelsCreateSVCEncoder(&openh264Encoder) != 0) {
                RTC_LOG(LS_ERROR) << "Failed to create OpenH264 encoder";
                RTC_DCHECK(!openh264Encoder);
                Release();
                ReportError();
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
            RTC_DCHECK(openh264Encoder);
            encoders[i] = openh264Encoder;

            configurations[i].simulcastIdx = idx;
            configurations[i].sending = false;
            configurations[i].width = codec.simulcastStream[idx].width;
            configurations[i].height = codec.simulcastStream[idx].height;
            configurations[i].maxFrameRate = static_cast<float>(codec.maxFramerate);
            configurations[i].frameDroppingOn = codec.GetFrameDropEnabled();
            configurations[i].keyFrameInterval = codec.H264()->keyFrameInterval;
            configurations[i].numTemporalLayers = std::max(codec.H264()->numberOfTemporalLayers, codec.simulcastStream[idx].numberOfTemporalLayers);
            if (i > 0) {
                downscaledBuffers[i - 1] = webrtc::I420Buffer::Create(
                    configurations[i].width, configurations[i].height,
                    configurations[i].width, configurations[i].width / 2,
                    configurations[i].width / 2
                );
            }
            configurations[i].maxBps = codec.maxBitrate * 1000;
            configurations[i].targetBps = codec.startBitrate * 1000;

            SEncParamExt encoderParams = CreateEncoderParams(i);

            if (openh264Encoder->InitializeExt(&encoderParams) != 0) {
                RTC_LOG(LS_ERROR) << "Failed to initialize OpenH264 encoder";
                Release();
                ReportError();
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
            int video_format = videoFormatI420;
            openh264Encoder->SetOption(ENCODER_OPTION_DATAFORMAT, &video_format);
            int32_t iTraceLevel = WELS_LOG_QUIET;
            openh264Encoder->SetOption(ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
            const size_t new_capacity = CalcBufferSize(
                webrtc::VideoType::kI420,
                codec.simulcastStream[idx].width,
                codec.simulcastStream[idx].height
            );
            encodedImages[i].SetEncodedData(webrtc::EncodedImageBuffer::Create(new_capacity));
            encodedImages[i]._encodedWidth = codec.simulcastStream[idx].width;
            encodedImages[i]._encodedHeight = codec.simulcastStream[idx].height;
            encodedImages[i].set_size(0);

            tl0syncLimit[i] = configurations[i].numTemporalLayers;
            scalabilityModes[i] = ScalabilityModeFromTemporalLayers(configurations[i].numTemporalLayers);
            if (scalabilityModes[i].has_value()) {
                svcControllers[i] = CreateScalabilityStructure(*scalabilityModes[i]);
                if (svcControllers[i] == nullptr) {
                  RTC_LOG(LS_ERROR) << "Failed to create scalability structure";
                  Release();
                  ReportError();
                  return WEBRTC_VIDEO_CODEC_ERROR;
                }
            }
        }

        webrtc::SimulcastRateAllocator initAllocator(env, codec);
        const webrtc::VideoBitrateAllocation allocation = initAllocator.Allocate(
            webrtc::VideoBitrateAllocationParameters(
                webrtc::DataRate::KilobitsPerSec(codec.startBitrate),
                codec.maxFramerate
            )
        );
        SetRates(RateControlParameters(allocation, codec.maxFramerate));
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t H264Encoder::RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) {
        encodedImageCallback = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t H264Encoder::Release() {
        while (!encoders.empty()) {
            if (ISVCEncoder* openh264Encoder = encoders.back()) {
                RTC_CHECK_EQ(0, openh264Encoder->Uninitialize());
                WelsDestroySVCEncoder(openh264Encoder);
            }
            encoders.pop_back();
        }
        downscaledBuffers.clear();
        configurations.clear();
        encodedImages.clear();
        pictures.clear();
        tl0syncLimit.clear();
        svcControllers.clear();
        scalabilityModes.clear();
        return WEBRTC_VIDEO_CODEC_OK;
    }

    int32_t H264Encoder::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frame_types) {
        if (encoders.empty()) {
            ReportError();
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        if (!encodedImageCallback) {
            RTC_LOG(LS_INFO)
                << "InitEncode() has been called, but a callback function "
                   "has not been set with RegisterEncodeCompleteCallback()";
            ReportError();
            return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
        }
        const rtc::scoped_refptr<webrtc::I420BufferInterface> frameBuffer = frame.video_frame_buffer()->ToI420();
        if (!frameBuffer) {
            RTC_LOG(LS_ERROR) << "Failed to convert "
                              << VideoFrameBufferTypeToString(frame.video_frame_buffer()->type())
                              << " image to I420. Can't encode frame.";
            return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
        }

        RTC_CHECK(frameBuffer->type() == webrtc::VideoFrameBuffer::Type::kI420 || frameBuffer->type() == webrtc::VideoFrameBuffer::Type::kI420A);

        bool is_keyframe_needed = false;
        for (auto & [simulcastIdx, width, height, sending, keyFrameRequest, maxFrameRate, targetBps, maxBps, frameDroppingOn, keyFrameInterval, numTemporalLayers] : configurations) {
            if (keyFrameRequest && sending) {
                is_keyframe_needed = true;
                break;
            }
        }

        RTC_DCHECK_EQ(configurations[0].width, frameBuffer->width());
        RTC_DCHECK_EQ(configurations[0].height, frameBuffer->height());

        for (size_t i = 0; i < encoders.size(); ++i) {
            pictures[i] = {};
            pictures[i].iPicWidth = configurations[i].width;
            pictures[i].iPicHeight = configurations[i].height;
            pictures[i].iColorFormat = videoFormatI420;
            pictures[i].uiTimeStamp = frame.ntp_time_ms();

            if (i == 0) {
                pictures[i].iStride[0] = frameBuffer->StrideY();
                pictures[i].iStride[1] = frameBuffer->StrideU();
                pictures[i].iStride[2] = frameBuffer->StrideV();
                pictures[i].pData[0] = const_cast<uint8_t*>(frameBuffer->DataY());
                pictures[i].pData[1] = const_cast<uint8_t*>(frameBuffer->DataU());
                pictures[i].pData[2] = const_cast<uint8_t*>(frameBuffer->DataV());
            } else {
                pictures[i].iStride[0] = downscaledBuffers[i - 1]->StrideY();
                pictures[i].iStride[1] = downscaledBuffers[i - 1]->StrideU();
                pictures[i].iStride[2] = downscaledBuffers[i - 1]->StrideV();
                pictures[i].pData[0] = const_cast<uint8_t*>(downscaledBuffers[i - 1]->DataY());
                pictures[i].pData[1] = const_cast<uint8_t*>(downscaledBuffers[i - 1]->DataU());
                pictures[i].pData[2] = const_cast<uint8_t*>(downscaledBuffers[i - 1]->DataV());
                I420Scale(
                    pictures[i - 1].pData[0], pictures[i - 1].iStride[0],
                    pictures[i - 1].pData[1], pictures[i - 1].iStride[1],
                    pictures[i - 1].pData[2], pictures[i - 1].iStride[2],
                    configurations[i - 1].width,
                    configurations[i - 1].height,
                    pictures[i].pData[0], pictures[i].iStride[0],
                    pictures[i].pData[1], pictures[i].iStride[1],
                    pictures[i].pData[2], pictures[i].iStride[2],
                    configurations[i].width,
                    configurations[i].height,
                    libyuv::kFilterBox
                );
            }

            if (!configurations[i].sending) {
                continue;
            }
            if (frame_types != nullptr && i < frame_types->size()) {
                if ((*frame_types)[i] == webrtc::VideoFrameType::kEmptyFrame) {
                    continue;
                }
            }

            const auto simulcast_idx = static_cast<size_t>(configurations[i].simulcastIdx);
            const bool sendKeyFrame = is_keyframe_needed || (frame_types && simulcast_idx < frame_types->size() &&  (*frame_types)[simulcast_idx] == webrtc::VideoFrameType::kVideoFrameKey);
            if (sendKeyFrame) {
                encoders[i]->ForceIntraFrame(true);
                configurations[i].keyFrameRequest = false;
            }

            SFrameBSInfo info = {};

            std::vector<webrtc::ScalableVideoController::LayerFrameConfig> layerFrames;
            if (svcControllers[i]) {
                layerFrames = svcControllers[i]->NextFrameConfig(sendKeyFrame);
                RTC_CHECK_EQ(layerFrames.size(), 1);
            }

            if (const int encRet = encoders[i]->EncodeFrame(&pictures[i], &info); encRet != 0) {
                RTC_LOG(LS_ERROR)
                    << "OpenH264 frame encoding failed, EncodeFrame returned " << encRet
                    << ".";
                ReportError();
                return WEBRTC_VIDEO_CODEC_ERROR;
            }

            encodedImages[i]._encodedWidth = configurations[i].width;
            encodedImages[i]._encodedHeight = configurations[i].height;
            encodedImages[i].SetRtpTimestamp(frame.rtp_timestamp());
            encodedImages[i].SetColorSpace(frame.color_space());
            encodedImages[i]._frameType = ConvertToVideoFrameType(info.eFrameType);
            encodedImages[i].SetSimulcastIndex(configurations[i].simulcastIdx);

            RtpFragmentize(&encodedImages[i], &info);

            if (encodedImages[i].size() > 0) {
                h264BitstreamParser.ParseBitstream(encodedImages[i]);
                encodedImages[i].qp_ = h264BitstreamParser.GetLastSliceQp().value_or(-1);
                webrtc::CodecSpecificInfo codec_specific;
                codec_specific.codecType = webrtc::kVideoCodecH264;
                codec_specific.codecSpecific.H264.packetization_mode = packetizationMode;
                codec_specific.codecSpecific.H264.temporal_idx = webrtc::kNoTemporalIdx;
                codec_specific.codecSpecific.H264.idr_frame = info.eFrameType == videoFrameTypeIDR;
                codec_specific.codecSpecific.H264.base_layer_sync = false;
                if (configurations[i].numTemporalLayers > 1) {
                    const uint8_t tid = info.sLayerInfo[0].uiTemporalId;
                    codec_specific.codecSpecific.H264.temporal_idx = tid;
                    codec_specific.codecSpecific.H264.base_layer_sync = tid > 0 && tid < tl0syncLimit[i];
                    if (svcControllers[i]) {
                        if (encodedImages[i]._frameType == webrtc::VideoFrameType::kVideoFrameKey) {
                            layerFrames = svcControllers[i]->NextFrameConfig(true);
                            RTC_CHECK_EQ(layerFrames.size(), 1);
                            RTC_DCHECK_EQ(layerFrames[0].TemporalId(), 0);
                            RTC_DCHECK_EQ(layerFrames[0].IsKeyframe(), true);
                        }
                        if (layerFrames[0].TemporalId() != tid) {
                            RTC_LOG(LS_INFO)
                                << "Encoder produced a frame with temporal id " << tid
                                << ", expected " << layerFrames[0].TemporalId() << ".";
                            continue;
                        }
                        encodedImages[i].SetTemporalIndex(tid);
                    }
                    if (codec_specific.codecSpecific.H264.base_layer_sync) {
                        tl0syncLimit[i] = tid;
                    }
                    if (tid == 0) {
                        tl0syncLimit[i] = configurations[i].numTemporalLayers;
                    }
                }
                if (svcControllers[i]) {
                    codec_specific.generic_frame_info = svcControllers[i]->OnEncodeDone(layerFrames[0]);
                    if (sendKeyFrame && codec_specific.generic_frame_info.has_value()) {
                        codec_specific.template_structure = svcControllers[i]->DependencyStructure();
                    }
                    codec_specific.scalability_mode = scalabilityModes[i];
                }
                encodedImageCallback->OnEncodedImage(encodedImages[i], &codec_specific);
            }
        }
        return WEBRTC_VIDEO_CODEC_OK;
    }

    void H264Encoder::SetRates(const RateControlParameters& parameters) {
        if (encoders.empty()) {
            RTC_LOG(LS_INFO) << "SetRates() while uninitialized.";
            return;
        }

        if (parameters.framerate_fps < 1.0) {
            RTC_LOG(LS_INFO) << "Invalid frame rate: " << parameters.framerate_fps;
            return;
        }

        if (parameters.bitrate.get_sum_bps() == 0) {
            for (auto & configuration : configurations) {
                configuration.SetStreamState(false);
            }
            return;
        }

        codec.maxFramerate = static_cast<uint32_t>(parameters.framerate_fps);

        size_t streamIdx = encoders.size() - 1;
        for (size_t i = 0; i < encoders.size(); ++i, --streamIdx) {
            configurations[i].targetBps = parameters.bitrate.GetSpatialLayerSum(streamIdx);
            configurations[i].maxFrameRate = static_cast<float>(parameters.framerate_fps);

            if (configurations[i].targetBps) {
                configurations[i].SetStreamState(true);

                // Update h264 encoder.
                SBitrateInfo target_bitrate = {};
                target_bitrate.iLayer = SPATIAL_LAYER_ALL,
                target_bitrate.iBitrate = static_cast<int>(configurations[i].targetBps);
                encoders[i]->SetOption(ENCODER_OPTION_BITRATE, &target_bitrate);
                encoders[i]->SetOption(ENCODER_OPTION_FRAME_RATE, &configurations[i].maxFrameRate);
            } else {
                configurations[i].SetStreamState(false);
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

    void H264Encoder::ReportError() {
        if (hasReportedError) return;
        RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.H264EncoderImpl.Event", kH264EncoderEventError, kH264EncoderEventMax);
        hasReportedError = true;
    }

    webrtc::VideoFrameType H264Encoder::ConvertToVideoFrameType(const EVideoFrameType type) {
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

    void H264Encoder::RtpFragmentize(webrtc::EncodedImage* encodedImage, SFrameBSInfo* info) {
        size_t requiredCapacity = 0;
        size_t fragments_count = 0;
        for (int layer = 0; layer < info->iLayerNum; ++layer) {
            const auto& [uiTemporalId, uiSpatialId, uiQualityId, eFrameType, uiLayerType, iSubSeqId, iNalCount, pNalLengthInByte, pBsBuf, rPsnr] = info->sLayerInfo[layer];
            for (int nal = 0; nal < iNalCount; ++nal, ++fragments_count) {
                RTC_CHECK_GE(pNalLengthInByte[nal], 0);
                RTC_CHECK_LE(pNalLengthInByte[nal], std::numeric_limits<size_t>::max() - requiredCapacity);
                requiredCapacity += pNalLengthInByte[nal];
            }
        }
        const auto buffer = webrtc::EncodedImageBuffer::Create(requiredCapacity);
        encodedImage->SetEncodedData(buffer);
        constexpr uint8_t startCode[4] = {0, 0, 0, 1};
        size_t frag = 0;
        encodedImage->set_size(0);
        for (int layer = 0; layer < info->iLayerNum; ++layer) {
            const auto& [uiTemporalId, uiSpatialId, uiQualityId, eFrameType, uiLayerType, iSubSeqId, iNalCount, pNalLengthInByte, pBsBuf, rPsnr] = info->sLayerInfo[layer];
            size_t layer_len = 0;
            for (int nal = 0; nal < iNalCount; ++nal, ++frag) {
                RTC_DCHECK_GE(pNalLengthInByte[nal], 4);
                RTC_DCHECK_EQ(pBsBuf[layer_len + 0], startCode[0]);
                RTC_DCHECK_EQ(pBsBuf[layer_len + 1], startCode[1]);
                RTC_DCHECK_EQ(pBsBuf[layer_len + 2], startCode[2]);
                RTC_DCHECK_EQ(pBsBuf[layer_len + 3], startCode[3]);
                layer_len += pNalLengthInByte[nal];
            }
            memcpy(buffer->data() + encodedImage->size(), pBsBuf, layer_len);
            encodedImage->set_size(encodedImage->size() + layer_len);
        }
    }

    void H264Encoder::ReportInit() {
        if (hasReportedInit)
            return;
        RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.H264EncoderImpl.Event", kH264EncoderEventInit, kH264EncoderEventMax);
        hasReportedInit = true;
    }

    std::optional<webrtc::ScalabilityMode> H264Encoder::ScalabilityModeFromTemporalLayers(const int numTemporalLayers) {
        switch (numTemporalLayers) {
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

    SEncParamExt H264Encoder::CreateEncoderParams(size_t i) const {
        SEncParamExt encoderParams;
        encoders[i]->GetDefaultParams(&encoderParams);
        if (codec.mode == webrtc::VideoCodecMode::kRealtimeVideo) {
            encoderParams.iUsageType = CAMERA_VIDEO_REAL_TIME;
        } else if (codec.mode == webrtc::VideoCodecMode::kScreensharing) {
            encoderParams.iUsageType = SCREEN_CONTENT_REAL_TIME;
        } else {
            RTC_DCHECK_NOTREACHED();
        }
        encoderParams.iPicWidth = configurations[i].width;
        encoderParams.iPicHeight = configurations[i].height;
        encoderParams.iTargetBitrate = static_cast<int>(configurations[i].targetBps);
        encoderParams.iMaxBitrate = UNSPECIFIED_BIT_RATE;
        encoderParams.iRCMode = RC_BITRATE_MODE;
        encoderParams.fMaxFrameRate = configurations[i].maxFrameRate;
        encoderParams.bEnableFrameSkip = configurations[i].frameDroppingOn;
        encoderParams.uiIntraPeriod = configurations[i].keyFrameInterval;
        encoderParams.eSpsPpsIdStrategy = SPS_LISTING;
        encoderParams.uiMaxNalSize = 0;
        encoderParams.iMultipleThreadIdc = NumberOfThreads(
            encoderThreadLimit,
            encoderParams.iPicWidth,
            encoderParams.iPicHeight,
            numberOfCores
        );
        encoderParams.sSpatialLayers[0].iVideoWidth = encoderParams.iPicWidth;
        encoderParams.sSpatialLayers[0].iVideoHeight = encoderParams.iPicHeight;
        encoderParams.sSpatialLayers[0].fFrameRate = encoderParams.fMaxFrameRate;
        encoderParams.sSpatialLayers[0].iSpatialBitrate = encoderParams.iTargetBitrate;
        encoderParams.sSpatialLayers[0].iMaxSpatialBitrate = encoderParams.iMaxBitrate;
        encoderParams.iTemporalLayerNum = configurations[i].numTemporalLayers;
        if (encoderParams.iTemporalLayerNum > 1) {
            encoderParams.iNumRefFrame = encoderParams.iTemporalLayerNum - 1;
        }
        RTC_LOG(LS_INFO) << "OpenH264 version is " << OPENH264_MAJOR << "." << OPENH264_MINOR;
        switch (packetizationMode) {
        case webrtc::H264PacketizationMode::SingleNalUnit:
            encoderParams.sSpatialLayers[0].sSliceArgument.uiSliceNum = 1;
            encoderParams.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SIZELIMITED_SLICE;
            encoderParams.sSpatialLayers[0].sSliceArgument.uiSliceSizeConstraint = static_cast<unsigned int>(maxPayloadSize);
            RTC_LOG(LS_INFO) << "Encoder is configured with NALU constraint: " << maxPayloadSize << " bytes";
            break;
        case webrtc::H264PacketizationMode::NonInterleaved:
            encoderParams.sSpatialLayers[0].sSliceArgument.uiSliceNum = 1;
            encoderParams.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
            break;
        }
        return encoderParams;
    }

    int H264Encoder::NumberOfThreads(const std::optional<int> encoderThreadLimit, const int width, const int height, const int numberOfCores) {
        if (encoderThreadLimit.has_value()) {
            const int limit = encoderThreadLimit.value();
            RTC_DCHECK_GE(limit, 1);
            if (width * height >= 1920 * 1080 && numberOfCores > 8) {
                return std::min(limit, 8);
            }
            if (width * height > 1280 * 960 && numberOfCores >= 6) {
                return std::min(limit, 3);
            }
            if (width * height > 640 * 480 && numberOfCores >= 3) {
                return std::min(limit, 2);
            }
            return 1;
        }
        return 1;
    }
} // openh264
#endif