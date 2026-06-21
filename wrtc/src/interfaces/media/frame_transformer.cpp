//
// Created by Laky64 on 06/06/26.
//

#include <common_video/h264/h264_bitstream_parser.h>
#include <common_video/h264/h264_common.h>
#include <wrtc/interfaces/media/frame_transformer.hpp>

namespace wrtc {
    FrameTransformer::FrameTransformer(
        const bool isEncryptor,
        E2EEncryptor* encryptor,
        const int64_t userId,
        const std::map<int32_t, PayloadType>& payloadTypeMapping,
        const std::function<std::pair<uint8_t, bool>()> &getAudioLevelAndSpeech,
        const std::function<void(uint8_t, bool)> &setAudioLevelAndSpeech
    ): isEncryptor(isEncryptor), userId(userId), encryptor(encryptor), payloadTypeMapping(payloadTypeMapping) {
        this->getAudioLevelAndSpeech = getAudioLevelAndSpeech;
        this->setAudioLevelAndSpeech = setAudioLevelAndSpeech;
    }

    void FrameTransformer::RegisterTransformedFrameSinkCallback(const webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback, const uint32_t ssrc) {
        webrtc::MutexLock lock(&mutex);
        sinkCallbackBySsrc[ssrc] = callback;
    }

    void FrameTransformer::UnregisterTransformedFrameSinkCallback(const uint32_t ssrc) {
        webrtc::MutexLock lock(&mutex);
        sinkCallbackBySsrc.erase(ssrc);
    }

    void FrameTransformer::RegisterTransformedFrameCallback(const webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback) {
        webrtc::MutexLock lock(&mutex);
        assert(sinkCallback == nullptr);
        sinkCallback = callback;
    }

    size_t FrameTransformer::calculateSliceHeaderBytesForPpsId(const uint8_t *data, const size_t size) {
        if (size < 2) {
            return 0;
        }
        auto rbsp = webrtc::H264::ParseRbsp(data, size);
        if (rbsp.size() < 2) {
            return 0;
        }

        const std::span<const uint8_t> rbspView(rbsp.data() + 1, rbsp.size() - 1);
        webrtc::BitstreamReader reader(rbspView);
        reader.ReadExponentialGolomb();

        if (!reader.Ok()) {
            return 4;
        }

        reader.ReadExponentialGolomb();
        if (!reader.Ok()) {
            return 4;
        }

        reader.ReadExponentialGolomb();
        if (!reader.Ok()) {
            return 4;
        }

        const size_t bitsConsumed = rbspView.size() * 8 - reader.RemainingBitCount();
        const size_t bytesRead = 1 + (bitsConsumed + 7) / 8;
        return bytesRead + 1;
    }

    bytes::binary FrameTransformer::calculateH264FramePlaintextHeaderSize(std::span<const uint8_t> frame, uint32_t &headerSize) {
        if (frame.empty()) {
            headerSize = 0;
            return {};
        }

        std::vector<webrtc::H264::NaluIndex> naluIndices = webrtc::H264::FindNaluIndices(frame);

        if (naluIndices.empty()) {
            headerSize = 0;

            bytes::binary frameData;
            frameData.resize(frame.size());
            std::ranges::copy(frame, frameData.begin());
            return frameData;
        }

        size_t maxOffset = 0;
        std::vector<uint8_t> naluToUpdate;

        for (const auto&[start_offset, payload_start_offset, payload_size] : naluIndices) {
            if (const auto startCodeLength = payload_start_offset - start_offset; startCodeLength == webrtc::H264::kNaluShortStartSequenceSize) {
                naluToUpdate.push_back(start_offset);
            }

            auto headerEndOffset = payload_start_offset + webrtc::H264::kNaluTypeSize;
            if (payload_size >= webrtc::H264::kNaluTypeSize) {
                if (const auto nalType = frame[payload_start_offset] & kTypeMask; nalType == webrtc::H264::kFuA) {
                    if (payload_size >= kFuAHeaderSize) {
                        headerEndOffset = payload_start_offset + kFuAHeaderSize;

                        if ((frame[payload_start_offset + 1] & 0x80) != 0) {
                            if (const uint8_t originalNalType = frame[payload_start_offset + 1] & kTypeMask; originalNalType == webrtc::H264::kIdr || originalNalType == 1) {
                                 headerEndOffset += 4;
                            }
                        }
                    }
                } else if (nalType == webrtc::H264::kStapA) {
                    if (payload_size >= kStapAHeaderSize) {
                        headerEndOffset = payload_start_offset + kStapAHeaderSize;

                        if (payload_size > kStapAHeaderSize) {
                            uint8_t firstNalType = frame[payload_start_offset + kStapAHeaderSize] & kTypeMask;
                            if (firstNalType == webrtc::H264::kIdr || firstNalType == 1) {
                                headerEndOffset += 4;
                            }
                        }
                    }
                } else if (nalType == webrtc::H264::kIdr || nalType == 1) {
                    const size_t ppsIdBytes = calculateSliceHeaderBytesForPpsId(
                        frame.data() + payload_start_offset,
                        payload_size
                    );

                    headerEndOffset = payload_start_offset + ppsIdBytes;
                    maxOffset = std::max(maxOffset, headerEndOffset);
                    break;
                } else if (nalType == webrtc::H264::kSps || nalType == webrtc::H264::kPps || nalType == webrtc::H264::kSei) {
                    headerEndOffset = payload_start_offset + payload_size;
                }
            }
            maxOffset = std::max(maxOffset, headerEndOffset);
        }

        bytes::binary frameData;
        frameData.resize(frame.size() + naluToUpdate.size());

        uint8_t offset = 0;
        for (uint8_t i = 0; i < static_cast<uint8_t>(naluToUpdate.size()); ++i) {
            const auto& naluIndex = naluToUpdate[i];
            if (naluIndex - offset > 0) {
                std::copy(frame.begin() + offset, frame.begin() + naluIndex, frameData.begin() + offset + i);
            }

            frameData[naluIndex + i] = 0;
            offset = naluIndex;
        }
        if (offset < frame.size()) {
            std::copy(frame.begin() + offset, frame.end(), frameData.begin() + offset + static_cast<uint8_t>(naluToUpdate.size()));
        }
        headerSize = static_cast<uint32_t>(maxOffset + naluToUpdate.size());
        return frameData;
    }

    bytes::binary FrameTransformer::calculateVp8FramePlaintextHeaderSize(std::span<const uint8_t> frame, uint32_t& headerSize) {
        if (frame.empty()) {
            headerSize = 0;
            return {};
        }

        if (const uint8_t firstByte = frame[0]; (firstByte & P_BIT) == 0) {
            headerSize = frame.size() >= 10 ? 10 : static_cast<uint32_t>(frame.size());
        } else {
            headerSize = 1;
        }

        bytes::binary frameData;
        frameData.resize(frame.size());
        std::ranges::copy(frame, frameData.begin());
        return frameData;
    }

    // ReSharper disable once CppDFAConstantParameter
    std::optional<FrameTransformer::IndexStartCodeSizePair> FrameTransformer::FindNextH26XNaluIndex(const uint8_t* buffer, const size_t bufferSize, const size_t searchStartIndex = 0) {
        if (bufferSize < kH26XNaluShortStartSequenceSize) {
            return std::nullopt;
        }

        for (size_t i = searchStartIndex; i < bufferSize - kH26XNaluShortStartSequenceSize;) {
            if (buffer[i + 2] > kH26XStartCodeHighestPossibleValue) {
                i += kH26XNaluShortStartSequenceSize;
            } else if (buffer[i + 2] == kH26XStartCodeEndByteValue) {
                if (buffer[i + 1] == kH26XStartCodeLeadingBytesValue && buffer[i] == kH26XStartCodeLeadingBytesValue) {
                    auto nalUnitStartIndex = i + kH26XNaluShortStartSequenceSize;
                    if (i >= 1 && buffer[i - 1] == kH26XStartCodeLeadingBytesValue) {
                        return IndexStartCodeSizePair({nalUnitStartIndex, 4});
                    }
                    return IndexStartCodeSizePair({nalUnitStartIndex, 3});
                }
                i += kH26XNaluShortStartSequenceSize;
            } else {
                ++i;
            }
        }
        return std::nullopt;
    }

    bool FrameTransformer::ValidateEncryptedFrame(const PayloadType payloadType, const std::span<const uint8_t> frame, uint32_t plaintextPrefix) {
        if (payloadType != PayloadType::H264) {
            return true;
        }

        static_assert(kH26XNaluShortStartSequenceSize - 1 >= 0, "Padding will overflow!");
        constexpr size_t Padding = kH26XNaluShortStartSequenceSize - 1;

        std::vector<UnencryptedRange> unencryptedRanges;
        if (plaintextPrefix != 0) {
            unencryptedRanges.emplace_back(0, plaintextPrefix);
        }

        size_t encryptedSectionStart = 0;
        for (const auto& range : unencryptedRanges) {
            if (encryptedSectionStart == range.offset) {
                encryptedSectionStart += range.size;
                continue;
            }

            const auto start = encryptedSectionStart - std::min(encryptedSectionStart, size_t{Padding});
            if (const auto end = std::min(range.offset + Padding, frame.size()); FindNextH26XNaluIndex(frame.data() + start, end - start)) {
                return false;
            }

            encryptedSectionStart = range.offset + range.size;
        }

        if (encryptedSectionStart == frame.size()) {
            return true;
        }

        const auto start = encryptedSectionStart - std::min(encryptedSectionStart, size_t{Padding});

        if (const auto end = frame.size(); FindNextH26XNaluIndex(frame.data() + start, end - start)) {
            return false;
        }
        return true;
    }

    void FrameTransformer::Transform(std::unique_ptr<webrtc::TransformableFrameInterface> frame) {
        webrtc::MutexLock lock(&mutex);
        const auto ssrc = frame->GetSsrc();
        const auto i = sinkCallbackBySsrc.find(ssrc);
        const auto sink = i != sinkCallbackBySsrc.end() && i->second ? i->second.get() : sinkCallback.get();
        if (!sink) {
            return;
        }

        auto payloadType = PayloadType::Unknown;
        if (const auto foundPayloadType = payloadTypeMapping.find(frame->GetPayloadType()); foundPayloadType != payloadTypeMapping.end()) {
            payloadType = foundPayloadType->second;
        }

        if (isEncryptor) {
            if (payloadType == PayloadType::H264 || payloadType == PayloadType::VP8) {
                uint32_t plaintextHeaderSize = 0;
                bytes::binary frameData;
                if (payloadType == PayloadType::H264) {
                    frameData = calculateH264FramePlaintextHeaderSize(frame->GetData(), plaintextHeaderSize);
                } else {
                    frameData = calculateVp8FramePlaintextHeaderSize(frame->GetData(), plaintextHeaderSize);
                }

                if (plaintextHeaderSize > static_cast<uint32_t>(frameData.size())) {
                    plaintextHeaderSize = static_cast<uint32_t>(frameData.size());
                }

                for (int attempt = 0; attempt < 4; attempt++) {
                    if (auto result = encryptor->encrypt(frameData, plaintextHeaderSize); !result.empty()) {
                        if (ValidateEncryptedFrame(payloadType, result, plaintextHeaderSize)) {
                            frame->SetData(result);
                            sink->OnTransformedFrame(std::move(frame));
                            break;
                        }
                    } else {
                        break;
                    }
                }
            } else {
                std::vector<uint8_t> buffer;
                buffer.resize(frame->GetData().size() + 1 + 1);
                std::copy(frame->GetData().begin(), frame->GetData().end(), buffer.begin());

                buffer[buffer.size() - 1 - 1] = 0x01;
                std::pair<uint8_t, bool> audioLevelAndSpeech = std::make_pair(0, false);
                if (const auto audioLevelAndSpeechOpt = getAudioLevelAndSpeech(); audioLevelAndSpeechOpt) {
                    audioLevelAndSpeech = *audioLevelAndSpeechOpt;
                }
                uint8_t encodedAudioLevelAndSpeech = 0;
                if (audioLevelAndSpeech.second) {
                    encodedAudioLevelAndSpeech = encodedAudioLevelAndSpeech | 0x80;
                }
                encodedAudioLevelAndSpeech |= audioLevelAndSpeech.first & 0x7f;
                buffer[buffer.size() - 1] = encodedAudioLevelAndSpeech;

                auto result = encryptor->encrypt(buffer, 0);
                if (!result.empty()) {
                    frame->SetData(result);
                    sink->OnTransformedFrame(std::move(frame));
                }
            }
        } else {
            if (payloadType != PayloadType::Opus) {
                std::vector<uint8_t> encryptedFrame;
                encryptedFrame.resize(frame->GetData().size());
                std::copy(frame->GetData().begin(), frame->GetData().end(), encryptedFrame.begin());

                auto decryptedFrame = encryptor->decrypt(userId, encryptedFrame);
                if (!decryptedFrame.empty()) {
                    frame->SetData(decryptedFrame);
                    sink->OnTransformedFrame(std::move(frame));
                }
            } else {
                std::vector<uint8_t> buffer;
                buffer.resize(frame->GetData().size());
                std::copy(frame->GetData().begin(), frame->GetData().end(), buffer.begin());

                if (auto result = encryptor->decrypt(userId, buffer); !result.empty()) {
                    if (result.size() >= 2) {
                        if (const uint8_t extensionFlags = result[result.size() - 2]; extensionFlags & 0x01) {
                            const uint8_t audioLevelAndSpeech = result[result.size() - 1];
                            const bool hasSpeech = (audioLevelAndSpeech & 0x80) != 0;
                            const uint8_t audioLevel = audioLevelAndSpeech & 0x7f;
                            (void) setAudioLevelAndSpeech(audioLevel, hasSpeech);

                            result.resize(result.size() - 2);
                        } else {
                            result.resize(result.size() - 1);
                        }
                    }

                    frame->SetData(result);
                    sink->OnTransformedFrame(std::move(frame));
                }
            }
        }
    }
} // wrtc