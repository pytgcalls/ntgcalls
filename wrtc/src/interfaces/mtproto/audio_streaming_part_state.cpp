//
// Created by Laky64 on 14/04/25.
//

#include <rtc_base/logging.h>
#include <wrtc/interfaces/mtproto/audio_streaming_part_state.hpp>

namespace wrtc {
    AudioStreamingPartState::AudioStreamingPartState(bytes::binary&& data, const std::string& container, const bool isSingleChannel) : isSingleChannel(isSingleChannel) {
        parsedPart = std::make_unique<AudioStreamingPartInternal>(std::move(data), container);
        if (parsedPart->getChannelUpdates().empty() && !isSingleChannel) {
            didReadToEnd = true;
            return;
        }

        remainingMilliseconds = parsedPart->getDurationInMilliseconds();

        for (const auto &it : parsedPart->getChannelUpdates()) {
            allSsrcs.insert(it.ssrc);
        }
    }

    AudioStreamingPartState::~AudioStreamingPartState() {
        parsedPart = nullptr;
    }

    std::vector<AudioStreamingPartState::Channel> AudioStreamingPartState::get10msPerChannel(AudioStreamingPartPersistentDecoder& persistentDecoder) {
        if (didReadToEnd) {
            return {};
        }

        for (const auto & [updateFrameIndex, id, ssrc] : parsedPart->getChannelUpdates()) {
            if (updateFrameIndex == frameIndex) {
                updateCurrentMapping(ssrc, id);
            }
        }

        auto [numSamples, numChannels] = parsedPart->readPcm(persistentDecoder, pcm10ms);
        if (numSamples <= 0) {
            didReadToEnd = true;
            return {};
        }

        std::vector<Channel> resultChannels;

        if (isSingleChannel) {
            for (int i = 0; i < numChannels; i++) {
                Channel emptyPart;
                emptyPart.ssrc = i + 1;
                resultChannels.push_back(emptyPart);
            }

            for (int i = 0; i < numChannels; i++) {
                const auto channel = resultChannels.begin() + i;
                const int sourceChannelIndex = i;
                for (int j = 0; j < numSamples; j++) {
                    channel->pcmData.push_back(pcm10ms[sourceChannelIndex + j * numChannels]);
                }
            }
        } else {
            for (const auto ssrc : allSsrcs) {
                Channel emptyPart;
                emptyPart.ssrc = ssrc;
                resultChannels.push_back(emptyPart);
            }

            for (auto & [ssrc, pcmData] : resultChannels) {
                if (auto mappedChannelIndex = getCurrentMappedChannelIndex(ssrc)) {
                    const int sourceChannelIndex = mappedChannelIndex.value();
                    for (int j = 0; j < numSamples; j++) {
                        pcmData.push_back(pcm10ms[sourceChannelIndex + j * numChannels]);
                    }
                } else {
                    for (int j = 0; j < numSamples; j++) {
                        pcmData.push_back(0);
                    }
                }
            }
        }

        remainingMilliseconds -= 10;
        if (remainingMilliseconds < 0) {
            remainingMilliseconds = 0;
        }
        frameIndex++;

        return resultChannels;
    }

    int AudioStreamingPartState::getRemainingMilliseconds() const {
        return remainingMilliseconds;
    }

    std::map<std::string, int32_t> AudioStreamingPartState::getEndpointMapping() const {
        return parsedPart->getEndpointMapping();
    }

    void AudioStreamingPartState::updateCurrentMapping(uint32_t ssrc, int channelIndex) {
        for (int i = static_cast<int>(currentChannelMapping.size()) - 1; i >= 0; i--) {
            const auto &entry = currentChannelMapping[i];
            if (entry.ssrc == ssrc && entry.channelIndex == channelIndex) {
                return;
            }
            if (entry.ssrc == ssrc || entry.channelIndex == channelIndex) {
                currentChannelMapping.erase(currentChannelMapping.begin() + i);
            }
        }
        currentChannelMapping.emplace_back(ssrc, channelIndex);
    }

    std::optional<int> AudioStreamingPartState::getCurrentMappedChannelIndex(const uint32_t ssrc) const {
        for (const auto &it : currentChannelMapping) {
            if (it.ssrc == ssrc) {
                return it.channelIndex;
            }
        }
        return std::nullopt;
    }
} // wrtc