//
// Created by Laky64 on 14/04/25.
//

#pragma once
#include <set>
#include <wrtc/interfaces/mtproto/audio_streaming_part_internal.hpp>

namespace wrtc {

    class AudioStreamingPartState {
        struct ChannelMapping {
            uint32_t ssrc = 0;
            int channelIndex = 0;

            ChannelMapping(const uint32_t ssrc, const int channelIndex) : ssrc(ssrc), channelIndex(channelIndex) {}
        };

        std::set<uint32_t> allSsrcs;
        bool didReadToEnd = false;
        bool isSingleChannel = false;
        int remainingMilliseconds = 0;
        std::vector<int16_t> pcm10ms;
        std::vector<ChannelMapping> currentChannelMapping;
        int frameIndex = 0;
        std::unique_ptr<AudioStreamingPartInternal> parsedPart;

        void updateCurrentMapping(uint32_t ssrc, int channelIndex);

        std::optional<int> getCurrentMappedChannelIndex(uint32_t ssrc) const;

    public:
        struct Channel {
            uint32_t ssrc = 0;
            std::vector<int16_t> pcmData;
        };

        AudioStreamingPartState(bytes::binary&& data, const std::string &container, bool isSingleChannel);

        ~AudioStreamingPartState();

        std::vector<Channel> get10msPerChannel(AudioStreamingPartPersistentDecoder &persistentDecoder);

        int getRemainingMilliseconds() const;

        std::map<std::string, int32_t> getEndpointMapping() const;
    };

} // wrtc
