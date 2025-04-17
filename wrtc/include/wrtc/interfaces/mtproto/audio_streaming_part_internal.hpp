//
// Created by Laky64 on 14/04/25.
//

#pragma once
#include <cstdint>
#include <map>
#include <sstream>
#include <wrtc/interfaces/mtproto/avio_context_impl.hpp>
#include <wrtc/interfaces/mtproto/audio_streaming_part_persistent_decoder.hpp>

namespace wrtc {

    class AudioStreamingPartInternal {
        struct ChannelUpdate {
            int frameIndex = 0;
            int id = 0;
            uint32_t ssrc = 0;
        };

        std::unique_ptr<AVIOContextImpl> avIoContext;

        AVFormatContext *inputFormatContext = nullptr;
        AVPacket packet = {};
        AVFrame *frame = nullptr;
        AVCodecParameters *audioCodecParameters = nullptr;

        bool didReadToEnd = false;

        int durationInMilliseconds = 0;
        int streamId = -1;
        int channelCount = 0;

        std::vector<ChannelUpdate> channelUpdates;
        std::map<std::string, int32_t> endpointMapping;

        std::vector<int16_t> pcmBuffer;
        int pcmBufferSampleOffset = 0;
        int pcmBufferSampleSize = 0;
        int readSampleCount = 0;

        template <typename Out>
        static void splitString(const std::string& s, const char delim, Out result) {
            std::istringstream iss(s);
            std::string item;
            while (std::getline(iss, item, delim)) {
                *result++ = item;
            }
        }

        static uint32_t stringToUInt32(const std::string& string);

        static std::optional<uint32_t> readInt32(const std::string& data, int &offset);

        static std::vector<ChannelUpdate> parseChannelUpdates(const std::string& data, int &offset);

        static int16_t sampleFloatToInt16(float sample);

        void fillPcmBuffer(AudioStreamingPartPersistentDecoder &persistentDecoder);

    public:
        struct ReadPcmResult {
            int numSamples = 0;
            int numChannels = 0;
        };

        AudioStreamingPartInternal(bytes::binary&& data, const std::string& container);

        ~AudioStreamingPartInternal();

        std::map<std::string, int32_t> getEndpointMapping() const;

        std::vector<ChannelUpdate> getChannelUpdates() const;

        int getDurationInMilliseconds() const;

        ReadPcmResult readPcm(AudioStreamingPartPersistentDecoder &persistentDecoder, std::vector<int16_t> &outPcm);
    };

} // wrtc
