//
// Created by Lauren on 20/11/26.
//

#include <third_party/opus/src/include/opus.h>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

namespace wrtc::audio_factory {
    int OpusPacket::samples(const uint8_t* data, const size_t size) {
        return opus_packet_get_nb_samples(data, static_cast<opus_int32>(size), kSampleRate);
    }

    size_t OpusPacket::frames(const uint8_t* data, const size_t size) {
        const int count = samples(data, size);
        return count > 0 ? static_cast<size_t>(count / kFrameSamples) : 0;
    }

    size_t OpusPacket::channels(const uint8_t* data) {
        return static_cast<size_t>(opus_packet_get_nb_channels(data));
    }

    size_t OpusPacket::channels(const webrtc::SdpAudioFormat& format) {
        const auto stereo = format.parameters.find("stereo");
        return stereo != format.parameters.end() && stereo->second == "1" ? 2 : 1;
    }
} // wrtc::audio_factory
