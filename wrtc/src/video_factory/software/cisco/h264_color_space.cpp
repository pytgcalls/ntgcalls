//
// Created by Laky64 on 06/11/24.
//

#ifndef IS_ANDROID
#include <wrtc/video_factory/software/openh264/h264_color_space.hpp>

namespace openh264 {
  webrtc::ColorSpace ExtractH264ColorSpace(const AVCodecContext* codec) {
    auto primaries = webrtc::ColorSpace::PrimaryID::kUnspecified;
    switch (codec->color_primaries) {
    case AVCOL_PRI_BT709:
      primaries = webrtc::ColorSpace::PrimaryID::kBT709;
      break;
    case AVCOL_PRI_BT470M:
      primaries = webrtc::ColorSpace::PrimaryID::kBT470M;
      break;
    case AVCOL_PRI_BT470BG:
      primaries = webrtc::ColorSpace::PrimaryID::kBT470BG;
      break;
    case AVCOL_PRI_SMPTE170M:
      primaries = webrtc::ColorSpace::PrimaryID::kSMPTE170M;
      break;
    case AVCOL_PRI_SMPTE240M:
      primaries = webrtc::ColorSpace::PrimaryID::kSMPTE240M;
      break;
    case AVCOL_PRI_FILM:
      primaries = webrtc::ColorSpace::PrimaryID::kFILM;
      break;
    case AVCOL_PRI_BT2020:
      primaries = webrtc::ColorSpace::PrimaryID::kBT2020;
      break;
    case AVCOL_PRI_SMPTE428:
      primaries = webrtc::ColorSpace::PrimaryID::kSMPTEST428;
      break;
    case AVCOL_PRI_SMPTE431:
      primaries = webrtc::ColorSpace::PrimaryID::kSMPTEST431;
      break;
    case AVCOL_PRI_SMPTE432:
      primaries = webrtc::ColorSpace::PrimaryID::kSMPTEST432;
      break;
    case AVCOL_PRI_JEDEC_P22:
      primaries = webrtc::ColorSpace::PrimaryID::kJEDECP22;
      break;
    case AVCOL_PRI_RESERVED0:
    case AVCOL_PRI_UNSPECIFIED:
    case AVCOL_PRI_RESERVED:
    default:
      break;
    }

    auto transfer = webrtc::ColorSpace::TransferID::kUnspecified;
    switch (codec->color_trc) {
    case AVCOL_TRC_BT709:
      transfer = webrtc::ColorSpace::TransferID::kBT709;
      break;
    case AVCOL_TRC_GAMMA22:
      transfer = webrtc::ColorSpace::TransferID::kGAMMA22;
      break;
    case AVCOL_TRC_GAMMA28:
      transfer = webrtc::ColorSpace::TransferID::kGAMMA28;
      break;
    case AVCOL_TRC_SMPTE170M:
      transfer = webrtc::ColorSpace::TransferID::kSMPTE170M;
      break;
    case AVCOL_TRC_SMPTE240M:
      transfer = webrtc::ColorSpace::TransferID::kSMPTE240M;
      break;
    case AVCOL_TRC_LINEAR:
      transfer = webrtc::ColorSpace::TransferID::kLINEAR;
      break;
    case AVCOL_TRC_LOG:
      transfer = webrtc::ColorSpace::TransferID::kLOG;
      break;
    case AVCOL_TRC_LOG_SQRT:
      transfer = webrtc::ColorSpace::TransferID::kLOG_SQRT;
      break;
    case AVCOL_TRC_IEC61966_2_4:
      transfer = webrtc::ColorSpace::TransferID::kIEC61966_2_4;
      break;
    case AVCOL_TRC_BT1361_ECG:
      transfer = webrtc::ColorSpace::TransferID::kBT1361_ECG;
      break;
    case AVCOL_TRC_IEC61966_2_1:
      transfer = webrtc::ColorSpace::TransferID::kIEC61966_2_1;
      break;
    case AVCOL_TRC_BT2020_10:
      transfer = webrtc::ColorSpace::TransferID::kBT2020_10;
      break;
    case AVCOL_TRC_BT2020_12:
      transfer = webrtc::ColorSpace::TransferID::kBT2020_12;
      break;
    case AVCOL_TRC_SMPTE2084:
      transfer = webrtc::ColorSpace::TransferID::kSMPTEST2084;
      break;
    case AVCOL_TRC_SMPTE428:
      transfer = webrtc::ColorSpace::TransferID::kSMPTEST428;
      break;
    case AVCOL_TRC_ARIB_STD_B67:
      transfer = webrtc::ColorSpace::TransferID::kARIB_STD_B67;
      break;
    case AVCOL_TRC_RESERVED0:
    case AVCOL_TRC_UNSPECIFIED:
    case AVCOL_TRC_RESERVED:
    default:
      break;
    }

    auto matrix = webrtc::ColorSpace::MatrixID::kUnspecified;
    switch (codec->colorspace) {
    case AVCOL_SPC_RGB:
      matrix = webrtc::ColorSpace::MatrixID::kRGB;
      break;
    case AVCOL_SPC_BT709:
      matrix = webrtc::ColorSpace::MatrixID::kBT709;
      break;
    case AVCOL_SPC_FCC:
      matrix = webrtc::ColorSpace::MatrixID::kFCC;
      break;
    case AVCOL_SPC_BT470BG:
      matrix = webrtc::ColorSpace::MatrixID::kBT470BG;
      break;
    case AVCOL_SPC_SMPTE170M:
      matrix = webrtc::ColorSpace::MatrixID::kSMPTE170M;
      break;
    case AVCOL_SPC_SMPTE240M:
      matrix = webrtc::ColorSpace::MatrixID::kSMPTE240M;
      break;
    case AVCOL_SPC_YCGCO:
      matrix = webrtc::ColorSpace::MatrixID::kYCOCG;
      break;
    case AVCOL_SPC_BT2020_NCL:
      matrix = webrtc::ColorSpace::MatrixID::kBT2020_NCL;
      break;
    case AVCOL_SPC_BT2020_CL:
      matrix = webrtc::ColorSpace::MatrixID::kBT2020_CL;
      break;
    case AVCOL_SPC_SMPTE2085:
      matrix = webrtc::ColorSpace::MatrixID::kSMPTE2085;
      break;
    case AVCOL_SPC_CHROMA_DERIVED_NCL:
    case AVCOL_SPC_CHROMA_DERIVED_CL:
    case AVCOL_SPC_ICTCP:
    case AVCOL_SPC_UNSPECIFIED:
    case AVCOL_SPC_RESERVED:
    default:
      break;
    }

    auto range = webrtc::ColorSpace::RangeID::kInvalid;
    switch (codec->color_range) {
    case AVCOL_RANGE_MPEG:
      range = webrtc::ColorSpace::RangeID::kLimited;
      break;
    case AVCOL_RANGE_JPEG:
      range = webrtc::ColorSpace::RangeID::kFull;
      break;
    case AVCOL_RANGE_UNSPECIFIED:
    default:
      break;
    }
    return {primaries, transfer, matrix, range};
  }
} // openh264

#endif