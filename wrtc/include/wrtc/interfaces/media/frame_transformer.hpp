//
// Created by Lauren on 06/06/26.
//

#pragma once
#include <map>
#include <api/frame_transformer_interface.h>
#include <rtc_base/synchronization/mutex.h>
#include <wrtc/interfaces/media/e2e_encryptor.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace wrtc::interfaces::media {
    class FrameTransformer: public webrtc::FrameTransformerInterface {
    public:
        enum class PayloadType {
            Unknown,
            Opus,
            H264,
            VP8
        };

        FrameTransformer(
            bool is_encryptor,
            E2EEncryptor* encryptor,
            int64_t user_id,
            const std::map<int32_t, PayloadType>& payload_type_mapping,
            const std::function<std::pair<uint8_t, bool>()>& get_audio_level_and_speech,
            const std::function<void(uint8_t, bool)>& set_audio_level_and_speech
        );

        void RegisterTransformedFrameSinkCallback(webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback, uint32_t ssrc) override;

        void UnregisterTransformedFrameSinkCallback(uint32_t ssrc) override;

        void RegisterTransformedFrameCallback(webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback) override;

        void Transform(std::unique_ptr<webrtc::TransformableFrameInterface> frame) override;

    private:
        using IndexStartCodeSizePair = std::pair<size_t, size_t>;

        struct UnencryptedRange {
            size_t offset = 0;
            size_t size = 0;

            UnencryptedRange(const size_t offset, const size_t size): offset(offset), size(size) {}
        };

        static constexpr uint8_t kH26XStartCodeEndByteValue = 1;
        static constexpr uint8_t kH26XStartCodeLeadingBytesValue = 0;
        static constexpr uint8_t kH26XNaluShortStartSequenceSize = 3;
        static constexpr uint8_t kH26XStartCodeHighestPossibleValue = 1;

        static constexpr uint8_t kPBit = 0x01;
        static constexpr uint8_t kTypeMask = 0x1F;
        static constexpr size_t kNalHeaderSize = 1;
        static constexpr size_t kFuAHeaderSize = 2;
        static constexpr size_t kLengthFieldSize = 2;
        static constexpr size_t kStapAHeaderSize = kNalHeaderSize + kLengthFieldSize;

        bool is_encryptor_;
        int64_t user_id_;
        webrtc::Mutex mutex_;
        E2EEncryptor* encryptor_;
        std::map<int32_t, PayloadType> payload_type_mapping_;
        webrtc::scoped_refptr<webrtc::TransformedFrameCallback> sink_callback_;
        utils::synchronized_callback<std::pair<uint8_t, bool>()> get_audio_level_and_speech_;
        utils::synchronized_callback<void(uint8_t, bool)> set_audio_level_and_speech_;
        std::map<uint32_t, webrtc::scoped_refptr<webrtc::TransformedFrameCallback>> sink_callback_by_ssrc_;

        static bytes::binary calculate_h264_frame_plaintext_header_size(bytes::const_span frame, uint32_t& header_size);

        static bytes::binary calculate_vp8_frame_plaintext_header_size(bytes::const_span frame, uint32_t& header_size);

        static size_t calculate_slice_header_bytes_for_pps_id(const bytes::byte* data, size_t size);

        static std::optional<IndexStartCodeSizePair> find_next_h26x_nalu_index(const bytes::byte* buffer, size_t buffer_size, size_t search_start_index);

        static bool validate_encrypted_frame(PayloadType payload_type, bytes::const_span frame, uint32_t plaintext_prefix);
    };
} // wrtc::interfaces::media
