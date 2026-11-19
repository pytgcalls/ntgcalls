//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <map>
#include <set>
#include <memory>
#include <api/video_codecs/sdp_video_format.h>
#include <api/video_codecs/video_decoder.h>
#include <api/video_codecs/video_encoder.h>
#include <ntgcalls/io/threaded_writer.hpp>
#include <ntgcalls/media/composer/video_composer.hpp>
#include <ntgcalls/media/containers/matroska/matroska_muxer.hpp>

namespace ntgcalls::io {

    class MatroskaWriter: public ThreadedWriter, public webrtc::DecodedImageCallback, public webrtc::EncodedImageCallback {
        static constexpr uint32_t kComposedBitrate = 2500000;
        static constexpr auto kKeyframeInterval = std::chrono::seconds(2);

        std::mutex state_mutex_;
        std::unique_ptr<media::containers::MatroskaMuxer> muxer_;
        std::optional<wrtc::models::Codec> codec_;
        std::optional<int64_t> source_ssrc_;
        bool composing_ = false;
        std::unique_ptr<media::composer::VideoComposer> composer_;
        std::map<int64_t, std::unique_ptr<webrtc::VideoDecoder>> decoders_;
        std::map<int64_t, wrtc::models::Codec> decoder_codecs_;
        std::set<int64_t> synchronized_;
        std::unique_ptr<webrtc::VideoEncoder> encoder_;
        int64_t decoding_ssrc_ = 0;
        uint32_t decode_timestamp_ = 0;
        uint32_t encode_timestamp_ = 0;
        std::optional<std::chrono::steady_clock::time_point> last_keyframe_;

        void forward(const wrtc::models::Frame& frame);

        void decode(const wrtc::models::Frame& frame);

        bool ensure_decoder(int64_t ssrc, wrtc::models::Codec codec);

        bool ensure_encoder();

        void write_frame(const uint8_t* data, size_t size, int64_t timestamp_ms, bool keyframe);

        static webrtc::SdpVideoFormat sdp_format(wrtc::models::Codec codec);

        static webrtc::VideoCodecType codec_type(wrtc::models::Codec codec);

    protected:
        std::optional<std::pair<bytes::unique_binary, size_t>> idle_data() override;

        std::chrono::nanoseconds idle_interval() const override;

    public:
        explicit MatroskaWriter(media::BaseSink* sink);

        ~MatroskaWriter() override;

        void send_frames(std::vector<wrtc::models::Frame> frames) override;

        void remove_source(int64_t ssrc) override;

        int32_t Decoded(webrtc::VideoFrame& decoded_image) override;

        Result OnEncodedImage(const webrtc::EncodedImage& encoded_image, const webrtc::CodecSpecificInfo* codec_specific_info) override;

        void OnFrameDropped(uint32_t rtp_timestamp, int spatial_id, bool is_end_of_temporal_unit) override;
    };

} // ntgcalls::io
