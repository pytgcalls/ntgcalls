//
// Created by Lauren on 06/06/26.
//

#include <common_video/h264/h264_bitstream_parser.h>
#include <common_video/h264/h264_common.h>
#include <wrtc/interfaces/media/frame_transformer.hpp>

namespace wrtc::interfaces::media {
    FrameTransformer::FrameTransformer(
        const bool is_encryptor,
        E2EEncryptor* encryptor,
        const int64_t user_id,
        const std::map<int32_t, PayloadType>& payload_type_mapping,
        const std::function<std::pair<uint8_t, bool>()>& get_audio_level_and_speech,
        const std::function<void(uint8_t, bool)>& set_audio_level_and_speech
    ): is_encryptor_(is_encryptor), user_id_(user_id), encryptor_(encryptor), payload_type_mapping_(payload_type_mapping) {
        this->get_audio_level_and_speech_ = get_audio_level_and_speech;
        this->set_audio_level_and_speech_ = set_audio_level_and_speech;
    }

    void FrameTransformer::RegisterTransformedFrameSinkCallback(const webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback, const uint32_t ssrc) {
        const webrtc::MutexLock lock(&mutex_);
        sink_callback_by_ssrc_[ssrc] = callback;
    }

    void FrameTransformer::UnregisterTransformedFrameSinkCallback(const uint32_t ssrc) {
        const webrtc::MutexLock lock(&mutex_);
        sink_callback_by_ssrc_.erase(ssrc);
    }

    void FrameTransformer::RegisterTransformedFrameCallback(const webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback) {
        const webrtc::MutexLock lock(&mutex_);
        assert(sinkCallback == nullptr);
        sink_callback_ = callback;
    }

    size_t FrameTransformer::calculate_slice_header_bytes_for_pps_id(const bytes::byte* data, const size_t size) {
        if (size < 2) {
            return 0;
        }
        auto rbsp = webrtc::H264::ParseRbsp(data, size);
        if (rbsp.size() < 2) {
            return 0;
        }

        const bytes::const_span rbsp_view(rbsp.data() + 1, rbsp.size() - 1);
        webrtc::BitstreamReader reader(rbsp_view);
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

        const size_t bits_consumed = rbsp_view.size() * 8 - reader.RemainingBitCount();
        const size_t bytes_read = 1 + (bits_consumed + 7) / 8;
        return bytes_read + 1;
    }

    bytes::binary FrameTransformer::calculate_h264_frame_plaintext_header_size(bytes::const_span frame, uint32_t& header_size) {
        if (frame.empty()) {
            header_size = 0;
            return {};
        }

        const std::vector<webrtc::H264::NaluIndex> nalu_indices = webrtc::H264::FindNaluIndices(frame);

        if (nalu_indices.empty()) {
            header_size = 0;

            bytes::binary frame_data;
            frame_data.resize(frame.size());
            std::ranges::copy(frame, frame_data.begin());
            return frame_data;
        }

        size_t max_offset = 0;
        std::vector<uint8_t> nalu_to_update;

        for (const auto& [start_offset, payload_start_offset, payload_size] : nalu_indices) {
            if (const auto start_code_length = payload_start_offset - start_offset; start_code_length == webrtc::H264::kNaluShortStartSequenceSize) {
                nalu_to_update.push_back(start_offset);
            }

            auto header_end_offset = payload_start_offset + webrtc::H264::kNaluTypeSize;
            if (payload_size >= webrtc::H264::kNaluTypeSize) {
                if (const auto nal_type = frame[payload_start_offset] & kTypeMask; nal_type == webrtc::H264::kFuA) {
                    if (payload_size >= kFuAHeaderSize) {
                        header_end_offset = payload_start_offset + kFuAHeaderSize;

                        if ((frame[payload_start_offset + 1] & 0x80) != 0) {
                            if (const uint8_t original_nal_type = frame[payload_start_offset + 1] & kTypeMask; original_nal_type == webrtc::H264::kIdr || original_nal_type == 1) {
                                header_end_offset += 4;
                            }
                        }
                    }
                } else if (nal_type == webrtc::H264::kStapA) {
                    if (payload_size >= kStapAHeaderSize) {
                        header_end_offset = payload_start_offset + kStapAHeaderSize;

                        if (payload_size > kStapAHeaderSize) {
                            if (const uint8_t first_nal_type = frame[payload_start_offset + kStapAHeaderSize] & kTypeMask; first_nal_type == webrtc::H264::kIdr || first_nal_type == 1) {
                                header_end_offset += 4;
                            }
                        }
                    }
                } else if (nal_type == webrtc::H264::kIdr || nal_type == 1) {
                    const size_t pps_id_bytes = calculate_slice_header_bytes_for_pps_id(
                        frame.data() + payload_start_offset,
                        payload_size
                    );

                    header_end_offset = payload_start_offset + pps_id_bytes;
                    max_offset = std::max(max_offset, header_end_offset);
                    break;
                } else if (nal_type == webrtc::H264::kSps || nal_type == webrtc::H264::kPps || nal_type == webrtc::H264::kSei) {
                    header_end_offset = payload_start_offset + payload_size;
                }
            }
            max_offset = std::max(max_offset, header_end_offset);
        }

        bytes::binary frame_data;
        frame_data.resize(frame.size() + nalu_to_update.size());

        uint8_t offset = 0;
        for (uint8_t i = 0; i < static_cast<uint8_t>(nalu_to_update.size()); ++i) {
            const auto& nalu_index = nalu_to_update[i];
            if (nalu_index - offset > 0) {
                std::copy(frame.begin() + offset, frame.begin() + nalu_index, frame_data.begin() + offset + i);
            }

            frame_data[nalu_index + i] = 0;
            offset = nalu_index;
        }
        if (offset < frame.size()) {
            std::copy(frame.begin() + offset, frame.end(), frame_data.begin() + offset + static_cast<uint8_t>(nalu_to_update.size()));
        }
        header_size = static_cast<uint32_t>(max_offset + nalu_to_update.size());
        return frame_data;
    }

    bytes::binary FrameTransformer::calculate_vp8_frame_plaintext_header_size(bytes::const_span frame, uint32_t& header_size) {
        if (frame.empty()) {
            header_size = 0;
            return {};
        }

        if (const uint8_t first_byte = frame[0]; (first_byte & kPBit) == 0) {
            header_size = frame.size() >= 10 ? 10 : static_cast<uint32_t>(frame.size());
        } else {
            header_size = 1;
        }

        bytes::binary frame_data;
        frame_data.resize(frame.size());
        std::ranges::copy(frame, frame_data.begin());
        return frame_data;
    }

    // ReSharper disable once CppDFAConstantParameter
    std::optional<FrameTransformer::IndexStartCodeSizePair> FrameTransformer::find_next_h26x_nalu_index(const bytes::byte* buffer, const size_t buffer_size, const size_t search_start_index = 0) {
        if (buffer_size < kH26XNaluShortStartSequenceSize) {
            return std::nullopt;
        }

        for (size_t i = search_start_index; i < buffer_size - kH26XNaluShortStartSequenceSize;) {
            if (buffer[i + 2] > kH26XStartCodeHighestPossibleValue) {
                i += kH26XNaluShortStartSequenceSize;
            } else if (buffer[i + 2] == kH26XStartCodeEndByteValue) {
                if (buffer[i + 1] == kH26XStartCodeLeadingBytesValue && buffer[i] == kH26XStartCodeLeadingBytesValue) {
                    auto nal_unit_start_index = i + kH26XNaluShortStartSequenceSize;
                    if (i >= 1 && buffer[i - 1] == kH26XStartCodeLeadingBytesValue) {
                        return IndexStartCodeSizePair({nal_unit_start_index, 4});
                    }
                    return IndexStartCodeSizePair({nal_unit_start_index, 3});
                }
                i += kH26XNaluShortStartSequenceSize;
            } else {
                ++i;
            }
        }
        return std::nullopt;
    }

    bool FrameTransformer::validate_encrypted_frame(const PayloadType payload_type, const bytes::const_span frame, uint32_t plaintext_prefix) {
        if (payload_type != PayloadType::H264) {
            return true;
        }

        static_assert(kH26XNaluShortStartSequenceSize - 1 >= 0, "Padding will overflow!");
        constexpr size_t kPadding = kH26XNaluShortStartSequenceSize - 1;

        std::vector<UnencryptedRange> unencrypted_ranges;
        if (plaintext_prefix != 0) {
            unencrypted_ranges.emplace_back(0, plaintext_prefix);
        }

        size_t encrypted_section_start = 0;
        for (const auto& range : unencrypted_ranges) {
            if (encrypted_section_start == range.offset) {
                encrypted_section_start += range.size;
                continue;
            }

            const auto start = encrypted_section_start - std::min(encrypted_section_start, size_t{kPadding});
            if (const auto end = std::min(range.offset + kPadding, frame.size()); find_next_h26x_nalu_index(frame.data() + start, end - start)) {
                return false;
            }

            encrypted_section_start = range.offset + range.size;
        }

        if (encrypted_section_start == frame.size()) {
            return true;
        }

        const auto start = encrypted_section_start - std::min(encrypted_section_start, size_t{kPadding});

        if (const auto end = frame.size(); find_next_h26x_nalu_index(frame.data() + start, end - start)) {
            return false;
        }
        return true;
    }

    void FrameTransformer::Transform(std::unique_ptr<webrtc::TransformableFrameInterface> frame) {
        const webrtc::MutexLock lock(&mutex_);
        const auto ssrc = frame->GetSsrc();
        const auto i = sink_callback_by_ssrc_.find(ssrc);
        const auto sink = i != sink_callback_by_ssrc_.end() && i->second ? i->second.get() : sink_callback_.get();
        if (!sink) {
            return;
        }

        auto payload_type = PayloadType::Unknown;
        if (const auto found_payload_type = payload_type_mapping_.find(frame->GetPayloadType()); found_payload_type != payload_type_mapping_.end()) {
            payload_type = found_payload_type->second;
        }

        if (is_encryptor_) {
            if (payload_type == PayloadType::H264 || payload_type == PayloadType::VP8) {
                uint32_t plaintext_header_size = 0;
                bytes::binary frame_data;
                if (payload_type == PayloadType::H264) {
                    frame_data = calculate_h264_frame_plaintext_header_size(frame->GetData(), plaintext_header_size);
                } else {
                    frame_data = calculate_vp8_frame_plaintext_header_size(frame->GetData(), plaintext_header_size);
                }

                if (plaintext_header_size > static_cast<uint32_t>(frame_data.size())) {
                    plaintext_header_size = static_cast<uint32_t>(frame_data.size());
                }

                for (int attempt = 0; attempt < 4; attempt++) {
                    if (auto result = encryptor_->encrypt(frame_data, plaintext_header_size); !result.empty()) {
                        if (validate_encrypted_frame(payload_type, result, plaintext_header_size)) {
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
                std::pair<uint8_t, bool> audio_level_and_speech = std::make_pair(0, false);
                if (const auto audio_level_and_speech_opt = get_audio_level_and_speech_(); audio_level_and_speech_opt) {
                    audio_level_and_speech = *audio_level_and_speech_opt;
                }
                uint8_t encoded_audio_level_and_speech = 0;
                if (audio_level_and_speech.second) {
                    encoded_audio_level_and_speech = encoded_audio_level_and_speech | 0x80;
                }
                encoded_audio_level_and_speech |= audio_level_and_speech.first & 0x7f;
                buffer[buffer.size() - 1] = encoded_audio_level_and_speech;

                auto result = encryptor_->encrypt(buffer, 0);
                if (!result.empty()) {
                    frame->SetData(result);
                    sink->OnTransformedFrame(std::move(frame));
                }
            }
        } else {
            if (payload_type != PayloadType::Opus) {
                bytes::binary encrypted_frame;
                encrypted_frame.resize(frame->GetData().size());
                std::copy(frame->GetData().begin(), frame->GetData().end(), encrypted_frame.begin());

                auto decrypted_frame = encryptor_->decrypt(user_id_, encrypted_frame);
                if (!decrypted_frame.empty()) {
                    frame->SetData(decrypted_frame);
                    sink->OnTransformedFrame(std::move(frame));
                }
            } else {
                bytes::binary buffer;
                buffer.resize(frame->GetData().size());
                std::copy(frame->GetData().begin(), frame->GetData().end(), buffer.begin());

                if (auto result = encryptor_->decrypt(user_id_, buffer); !result.empty()) {
                    if (result.size() >= 2) {
                        if (const uint8_t extension_flags = result[result.size() - 2]; extension_flags & 0x01) {
                            const uint8_t audio_level_and_speech = result[result.size() - 1];
                            const bool has_speech = (audio_level_and_speech & 0x80) != 0;
                            const uint8_t audio_level = audio_level_and_speech & 0x7f;
                            (void) set_audio_level_and_speech_(audio_level, has_speech);

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
} // wrtc::interfaces::media
