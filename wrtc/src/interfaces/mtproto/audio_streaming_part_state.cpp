//
// Created by Lauren on 14/04/25.
//

#include <rtc_base/logging.h>
#include <wrtc/interfaces/mtproto/audio_streaming_part_state.hpp>

namespace wrtc::interfaces::mtproto {
    AudioStreamingPartState::AudioStreamingPartState(bytes::binary&& data, const std::string& container, const bool is_single_channel) : is_single_channel_(is_single_channel) {
        parsed_part_ = std::make_unique<AudioStreamingPartInternal>(std::move(data), container);
        if (parsed_part_->get_channel_updates().empty() && !is_single_channel) {
            did_read_to_end_ = true;
            return;
        }

        remaining_milliseconds_ = parsed_part_->get_duration_in_milliseconds();

        for (const auto &it : parsed_part_->get_channel_updates()) {
            all_ssrcs_.insert(it.ssrc);
        }
    }

    AudioStreamingPartState::~AudioStreamingPartState() {
        parsed_part_ = nullptr;
    }

    std::vector<AudioStreamingPartState::Channel> AudioStreamingPartState::get_10ms_per_channel(AudioStreamingPartPersistentDecoder& persistent_decoder) {
        if (did_read_to_end_) {
            return {};
        }

        for (const auto & [updateFrameIndex, id, ssrc] : parsed_part_->get_channel_updates()) {
            if (updateFrameIndex == frame_index_) {
                update_current_mapping(ssrc, id);
            }
        }

        auto [numSamples, numChannels] = parsed_part_->read_pcm(persistent_decoder, pcm10ms_);
        if (numSamples <= 0) {
            did_read_to_end_ = true;
            return {};
        }

        std::vector<Channel> result_channels;

        if (is_single_channel_) {
            for (int i = 0; i < numChannels; i++) {
                Channel empty_part;
                empty_part.ssrc = i + 1;
                result_channels.push_back(empty_part);
            }

            for (int i = 0; i < numChannels; i++) {
                const auto channel = result_channels.begin() + i;
                const int source_channel_index = i;
                for (int j = 0; j < numSamples; j++) {
                    channel->pcm_data.push_back(pcm10ms_[source_channel_index + j * numChannels]);
                }
            }
        } else {
            for (const auto ssrc : all_ssrcs_) {
                Channel empty_part;
                empty_part.ssrc = ssrc;
                result_channels.push_back(empty_part);
            }

            for (auto & [ssrc, pcmData] : result_channels) {
                if (auto mapped_channel_index = get_current_mapped_channel_index(ssrc)) {
                    const int source_channel_index = mapped_channel_index.value();
                    for (int j = 0; j < numSamples; j++) {
                        pcmData.push_back(pcm10ms_[source_channel_index + j * numChannels]);
                    }
                } else {
                    for (int j = 0; j < numSamples; j++) {
                        pcmData.push_back(0);
                    }
                }
            }
        }

        remaining_milliseconds_ -= 10;
        if (remaining_milliseconds_ < 0) {
            remaining_milliseconds_ = 0;
        }
        frame_index_++;

        return result_channels;
    }

    int AudioStreamingPartState::get_remaining_milliseconds() const {
        return remaining_milliseconds_;
    }

    std::map<std::string, int32_t> AudioStreamingPartState::get_endpoint_mapping() const {
        return parsed_part_->get_endpoint_mapping();
    }

    void AudioStreamingPartState::update_current_mapping(uint32_t ssrc, int channel_index) {
        for (int i = static_cast<int>(current_channel_mapping_.size()) - 1; i >= 0; i--) {
            const auto &entry = current_channel_mapping_[i];
            if (entry.ssrc == ssrc && entry.channel_index == channel_index) {
                return;
            }
            if (entry.ssrc == ssrc || entry.channel_index == channel_index) {
                current_channel_mapping_.erase(current_channel_mapping_.begin() + i);
            }
        }
        current_channel_mapping_.emplace_back(ssrc, channel_index);
    }

    std::optional<int> AudioStreamingPartState::get_current_mapped_channel_index(const uint32_t ssrc) const {
        for (const auto &it : current_channel_mapping_) {
            if (it.ssrc == ssrc) {
                return it.channel_index;
            }
        }
        return std::nullopt;
    }
} // wrtc::interfaces::mtproto