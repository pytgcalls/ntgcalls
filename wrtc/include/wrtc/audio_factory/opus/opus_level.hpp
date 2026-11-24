//
// Created by Lauren on 20/11/26.
//

#pragma once
#include <array>
#include <cstdint>
#include <third_party/opus/src/include/opus.h>
#include <third_party/opus/src/include/opus_custom.h>

extern "C" {
#include <third_party/opus/src/celt/laplace.h>

int clt_compute_allocation(const OpusCustomMode* m, int start, int end, const int* offsets, const int* cap, int alloc_trim, int* intensity, int* dual_stereo, opus_int32 total, opus_int32* balance, int* pulses, int* ebits, int* fine_priority, int C, int LM, ec_ctx* ec, int encode, int prev, int signalBandwidth);
}

namespace wrtc::audio_factory {
    class OpusLevel {
        static constexpr int kBands = 21;
        static constexpr int kMaxPacketSamples = 5760;
        static constexpr int kMaxFrames = 48;
        static constexpr double kPowerScale = 0.522;
        static constexpr std::array<int, kBands + 1> kBandEdges = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 34, 40, 48, 60, 78, 100};
        static constexpr std::array<float, kBands> kBandMeans = {6.4375f, 6.25f, 5.75f, 5.3125f, 5.0625f, 4.8125f, 4.5f, 4.375f, 4.875f, 4.6875f, 4.5625f, 4.4375f, 4.875f, 4.625f, 4.3125f, 4.5f, 4.375f, 4.625f, 4.75f, 4.4375f, 3.75f};
        static constexpr std::array<double, kBands> kDeemphasisGain = {44.068, 41.932, 38.225, 33.746, 29.184, 24.965, 21.275, 18.149, 14.462, 10.855, 8.362, 6.603, 4.858, 3.392, 2.504, 1.825, 1.324, 0.977, 0.694, 0.478, 0.349};
        static constexpr std::array<float, 4> kPredictionCoef = {29440 / 32768.f, 26112 / 32768.f, 21248 / 32768.f, 16384 / 32768.f};
        static constexpr std::array<float, 4> kBetaCoef = {30147 / 32768.f, 22282 / 32768.f, 12124 / 32768.f, 6554 / 32768.f};
        static constexpr float kBetaIntra = 4915 / 32768.f;
        static constexpr unsigned char kSmallEnergyIcdf[3] = {2, 1, 0};
        static constexpr unsigned char kTapsetIcdf[3] = {2, 1, 0};
        static constexpr unsigned char kSpreadIcdf[4] = {25, 23, 2, 0};
        static constexpr unsigned char kTrimIcdf[11] = {126, 124, 119, 109, 87, 41, 19, 9, 4, 2, 0};
        static constexpr signed char kTfSelect[4][8] = {
            {0, -1, 0, -1, 0, -1, 0, -1},
            {0, -1, 0, -2, 1, 0, 1, -1},
            {0, -2, 0, -3, 2, 0, 1, -1},
            {0, -2, 0, -3, 3, 0, 1, -1},
        };
        static constexpr unsigned char kProbModel[4][2][42] = {
            {
                {72, 127, 65, 129, 66, 128, 65, 128, 64, 128, 62, 128, 64, 128, 64, 128, 92, 78, 92, 79, 92, 78, 90, 79, 116, 41, 115, 40, 114, 40, 132, 26, 132, 26, 145, 17, 161, 12, 176, 10, 177, 11},
                {24, 179, 48, 138, 54, 135, 54, 132, 53, 134, 56, 133, 55, 132, 55, 132, 61, 114, 70, 96, 74, 88, 75, 88, 87, 74, 89, 66, 91, 67, 100, 59, 108, 50, 120, 40, 122, 37, 97, 43, 78, 50},
            },
            {
                {83, 78, 84, 81, 88, 75, 86, 74, 87, 71, 90, 73, 93, 74, 93, 74, 109, 40, 114, 36, 117, 34, 117, 34, 143, 17, 145, 18, 146, 19, 162, 12, 165, 10, 178, 7, 189, 6, 190, 8, 177, 9},
                {23, 178, 54, 115, 63, 102, 66, 98, 69, 99, 74, 89, 71, 91, 73, 91, 78, 89, 86, 80, 92, 66, 93, 64, 102, 59, 103, 60, 104, 60, 117, 52, 123, 44, 138, 35, 133, 31, 97, 38, 77, 45},
            },
            {
                {61, 90, 93, 60, 105, 42, 107, 41, 110, 45, 116, 38, 113, 38, 112, 38, 124, 26, 132, 27, 136, 19, 140, 20, 155, 14, 159, 16, 158, 18, 170, 13, 177, 10, 187, 8, 192, 6, 175, 9, 159, 10},
                {21, 178, 59, 110, 71, 86, 75, 85, 84, 83, 91, 66, 88, 73, 87, 72, 92, 75, 98, 72, 105, 58, 107, 54, 115, 52, 114, 55, 112, 56, 129, 51, 132, 40, 150, 33, 140, 29, 98, 35, 77, 42},
            },
            {
                {42, 121, 96, 66, 108, 43, 111, 40, 117, 44, 123, 32, 120, 36, 119, 33, 127, 33, 134, 34, 139, 21, 147, 23, 152, 20, 158, 25, 154, 26, 166, 21, 173, 16, 184, 13, 184, 10, 150, 13, 139, 15},
                {22, 178, 63, 114, 74, 82, 84, 83, 92, 82, 103, 62, 96, 72, 96, 67, 101, 73, 107, 72, 113, 55, 118, 52, 125, 52, 118, 52, 117, 55, 135, 49, 137, 39, 157, 32, 145, 29, 97, 33, 77, 40},
            },
        };
        static constexpr unsigned char kCaps[168] = {
            224, 224, 224, 224, 224, 224, 224, 224, 160, 160, 160, 160, 185, 185, 185, 178, 178, 168, 134, 61, 37,
            224, 224, 224, 224, 224, 224, 224, 224, 240, 240, 240, 240, 207, 207, 207, 198, 198, 183, 144, 66, 40,
            160, 160, 160, 160, 160, 160, 160, 160, 185, 185, 185, 185, 193, 193, 193, 183, 183, 172, 138, 64, 38,
            240, 240, 240, 240, 240, 240, 240, 240, 207, 207, 207, 207, 204, 204, 204, 193, 193, 180, 143, 66, 40,
            185, 185, 185, 185, 185, 185, 185, 185, 193, 193, 193, 193, 193, 193, 193, 183, 183, 172, 138, 65, 39,
            207, 207, 207, 207, 207, 207, 207, 207, 204, 204, 204, 204, 201, 201, 201, 188, 188, 176, 141, 66, 40,
            193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 193, 194, 194, 194, 184, 184, 173, 139, 65, 39,
            204, 204, 204, 204, 204, 204, 204, 204, 201, 201, 201, 201, 198, 198, 198, 187, 187, 175, 140, 66, 40,
        };

        int channels_;
        const OpusCustomMode* mode_ = nullptr;
        OpusDecoder* decoder_ = nullptr;
        std::array<std::array<float, kBands>, 2> energy_{};

        static int end_band(int bandwidth);

        static int frame_lm(int samples);

        static bool parse_transient(ec_dec* dec, int lm, int total);

        void parse_coarse(ec_dec* dec, int lm, int channels, int end);

        void parse_fine(ec_dec* dec, int lm, int channels, int end, int size, bool transient);

        double celt_power(const unsigned char* data, int size, int lm, int channels, int end);

        double decoded_power(const uint8_t* data, size_t size);

    public:
        explicit OpusLevel(size_t channels);

        ~OpusLevel();

        int16_t amplitude(const uint8_t* data, size_t size);
    };
} // wrtc::audio_factory
