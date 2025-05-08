//
// Created by Laky64 on 15/04/25.
//

#include <wrtc/interfaces/mtproto/video_streaming_part_state.hpp>

namespace wrtc {
    VideoStreamingPartState::VideoStreamingPartState(bytes::binary&& data, const webrtc::MediaType mediaType) {
        streamInfo = consumeStreamInfo(data);
        if (!streamInfo) {
            return;
        }
        for (size_t i = 0; i < streamInfo->events.size(); i++) {
            if (streamInfo->events[i].offset < 0) {
                continue;
            }
            size_t endOffset = 0;
            if (i == streamInfo->events.size() - 1) {
                endOffset = data.size();
            } else {
                endOffset = streamInfo->events[i + 1].offset;
            }
            if (endOffset <= streamInfo->events[i].offset) {
                continue;
            }
            if (endOffset > data.size()) {
                continue;
            }
            bytes::binary dataSlice(data.begin() + streamInfo->events[i].offset, data.begin() + static_cast<ptrdiff_t>(endOffset));
            webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0;
            switch (streamInfo->events[i].rotation) {
                case 0: {
                    rotation = webrtc::VideoRotation::kVideoRotation_0;
                    break;
                }
                case 90: {
                    rotation = webrtc::VideoRotation::kVideoRotation_90;
                    break;
                }
                case 180: {
                    rotation = webrtc::VideoRotation::kVideoRotation_180;
                    break;
                }
                case 270: {
                    rotation = webrtc::VideoRotation::kVideoRotation_270;
                    break;
                }
                default: {
                    break;
                }
            }

            switch (mediaType) {
                case webrtc::MediaType::AUDIO: {
                    auto part = std::make_unique<AudioStreamingPart>(std::move(dataSlice), streamInfo->container, true);
                    parsedAudioParts.push_back(std::move(part));
                } break;
                case webrtc::MediaType::VIDEO: {
                    auto part = std::make_unique<VideoStreamingPartInternal>(streamInfo->events[i].endpointId, rotation, std::move(dataSlice), streamInfo->container);
                    parsedVideoParts.push_back(std::move(part));
                } break;
                default: {
                    break;
                }
            }
        }
    }

    VideoStreamingPartState::~VideoStreamingPartState() {
        parsedAudioParts.clear();
        parsedVideoParts.clear();
        availableFrames.clear();
        streamInfo = std::nullopt;
    }

    std::optional<VideoStreamingPartFrame> VideoStreamingPartState::getFrameAtRelativeTimestamp(VideoStreamingSharedState* sharedState, const double timestamp) {
        while (true) {
            while (availableFrames.size() >= 2) {
                if (timestamp >= availableFrames[1].pts) {
                    availableFrames.erase(availableFrames.begin());
                } else {
                    break;
                }
            }

            if (availableFrames.size() < 2) {
                if (!parsedVideoParts.empty()) {
                    if (auto result = parsedVideoParts[0]->getNextFrame(sharedState)) {
                        availableFrames.push_back(result.value());
                    } else {
                        parsedVideoParts.erase(parsedVideoParts.begin());
                    }
                    continue;
                }
            }

            if (!availableFrames.empty()) {
                for (size_t i = 1; i < availableFrames.size(); i++) {
                    if (timestamp < availableFrames[i].pts) {
                        return availableFrames[i - 1];
                    }
                }
                return availableFrames[availableFrames.size() - 1];
            }
            return std::nullopt;
        }
    }

    std::optional<std::string> VideoStreamingPartState::getActiveEndpointId() const {
        if (!parsedVideoParts.empty()) {
            return parsedVideoParts[0]->getEndpointId();
        }
        return std::nullopt;
    }

    bool VideoStreamingPartState::hasRemainingFrames() const {
        return !parsedVideoParts.empty();
    }

    std::vector<AudioStreamingPartState::Channel> VideoStreamingPartState::getAudio10msPerChannel(AudioStreamingPartPersistentDecoder& persistentDecoder) {
        while (!parsedAudioParts.empty()) {
            if (auto firstPartResult = parsedAudioParts[0]->get10msPerChannel(persistentDecoder); firstPartResult.empty()) {
                parsedAudioParts.erase(parsedAudioParts.begin());
            } else {
                return firstPartResult;
            }
        }
        return {};
    }

    std::optional<int32_t> VideoStreamingPartState::readInt32(const bytes::binary& data, int& offset) {
        if (offset + 4 > data.size()) {
            return std::nullopt;
        }
        int32_t value = 0;
        memcpy(&value, data.data() + offset, 4);
        offset += 4;
        return value;
    }

    std::optional<uint8_t> VideoStreamingPartState::readBytesAsInt32(const bytes::binary& data, int& offset, const int count) {
        if (offset + count > data.size()) {
            return std::nullopt;
        }

        int32_t value = 0;
        memcpy(&value, data.data() + offset, count);
        offset += count;
        return value;
    }

    int32_t VideoStreamingPartState::roundUp(const int32_t numToRound) {
        const int32_t remainder = numToRound % 4;
        if (remainder == 0) {
            return numToRound;
        }
        return numToRound + 4 - remainder;
    }

    std::optional<std::string> VideoStreamingPartState::readSerializedString(const bytes::binary& data, int& offset) {
        if (const auto tmp = readBytesAsInt32(data, offset, 1)) {
            int paddingBytes = 0;
            int length = 0;
            if (tmp.value() == 254) {
                if (const auto len = readBytesAsInt32(data, offset, 3)) {
                    length = len.value();
                    paddingBytes = roundUp(length) - length;
                } else {
                    return std::nullopt;
                }
            }
            else {
                length = tmp.value();
                paddingBytes = roundUp(length + 1) - (length + 1);
            }

            if (offset + length > data.size()) {
                return std::nullopt;
            }

            std::string result(data.data() + offset, data.data() + offset + length);

            offset += length;
            offset += paddingBytes;

            return result;
        }
        return std::nullopt;
    }

    std::optional<VideoStreamingPartState::StreamEvent> VideoStreamingPartState::readVideoStreamEvent(const bytes::binary& data, int& offset) {
        StreamEvent event;

        if (const auto offsetValue = readInt32(data, offset)) {
            event.offset = offsetValue.value();
        } else {
            return std::nullopt;
        }

        if (const auto endpointId = readSerializedString(data, offset)) {
            event.endpointId = endpointId.value();
        } else {
            return std::nullopt;
        }

        if (const auto rotation = readInt32(data, offset)) {
            event.rotation = rotation.value();
        } else {
            return std::nullopt;
        }

        if (const auto extra = readInt32(data, offset)) {
            event.extra = extra.value();
        } else {
            return std::nullopt;
        }

        return event;
    }

    std::optional<VideoStreamingPartState::StreamInfo> VideoStreamingPartState::consumeStreamInfo(bytes::binary& data) {
        int offset = 0;
        if (const auto signature = readInt32(data, offset)) {
            if (signature.value() != 0xa12e810d) {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }

        StreamInfo info;

        if (const auto container = readSerializedString(data, offset)) {
            info.container = container.value();
        } else {
            return std::nullopt;
        }

        if (const auto activeMask = readInt32(data, offset)) {
            info.activeMask = activeMask.value();
        } else {
            return std::nullopt;
        }

        if (const auto eventCount = readInt32(data, offset)) {
            if (eventCount > 0) {
                if (const auto event = readVideoStreamEvent(data, offset)) {
                    info.events.push_back(event.value());
                } else {
                    return std::nullopt;
                }
            } else {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }
        data.erase(data.begin(), data.begin() + offset);
        return info;
    }
} // wrtc