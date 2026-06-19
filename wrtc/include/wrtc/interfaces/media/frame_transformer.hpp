//
// Created by Laky64 on 06/06/26.
//

#pragma once
#include <map>
#include <api/frame_transformer_interface.h>
#include <rtc_base/synchronization/mutex.h>
#include <wrtc/interfaces/media/e2e_encryptor.hpp>
#include <wrtc/utils/binary.hpp>
#include <wrtc/utils/synchronized_callback.hpp>

namespace wrtc {
    class FrameTransformer : public webrtc::FrameTransformerInterface  {
    public:
        enum class PayloadType {
            Unknown,
            Opus,
            H264,
            VP8
        };

        FrameTransformer(
            bool isEncryptor,
            E2EEncryptor* encryptor,
            int64_t userId,
            const std::map<int32_t, PayloadType>& payloadTypeMapping,
            const std::function<std::pair<uint8_t, bool>()> &getAudioLevelAndSpeech,
            const std::function<void(uint8_t, bool)> &setAudioLevelAndSpeech
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

            UnencryptedRange(const size_t offset, const size_t size) : offset(offset), size(size) {}
        };

        static constexpr uint8_t kH26XStartCodeEndByteValue = 1;
        static constexpr uint8_t kH26XStartCodeLeadingBytesValue = 0;
        static constexpr uint8_t kH26XNaluShortStartSequenceSize = 3;
        static constexpr uint8_t kH26XStartCodeHighestPossibleValue = 1;

        static constexpr uint8_t P_BIT = 0x01;
        static constexpr uint8_t kTypeMask = 0x1F;
        static constexpr size_t kNalHeaderSize = 1;
        static constexpr size_t kFuAHeaderSize = 2;
        static constexpr size_t kLengthFieldSize = 2;
        static constexpr size_t kStapAHeaderSize = kNalHeaderSize + kLengthFieldSize;

        bool isEncryptor;
        int64_t userId;
        webrtc::Mutex mutex;
        E2EEncryptor* encryptor;
        std::map<int32_t, PayloadType> payloadTypeMapping;
        webrtc::scoped_refptr<webrtc::TransformedFrameCallback> sinkCallback;
        synchronized_callback<std::pair<uint8_t, bool>()> getAudioLevelAndSpeech;
        synchronized_callback<void(uint8_t, bool)> setAudioLevelAndSpeech;
        std::map<uint32_t, webrtc::scoped_refptr<webrtc::TransformedFrameCallback>> sinkCallbackBySsrc;

        static bytes::binary calculateH264FramePlaintextHeaderSize(std::span<const uint8_t> frame, uint32_t& headerSize);

        static bytes::binary calculateVp8FramePlaintextHeaderSize(std::span<const uint8_t> frame, uint32_t &headerSize);

        static size_t calculateSliceHeaderBytesForPpsId(const uint8_t* data, size_t size);

        static std::optional<IndexStartCodeSizePair> FindNextH26XNaluIndex(const uint8_t *buffer, size_t bufferSize, size_t searchStartIndex);

        static bool ValidateEncryptedFrame(PayloadType payloadType, std::span<const uint8_t> frame, uint32_t plaintextPrefix);
    };
} // wrtc
