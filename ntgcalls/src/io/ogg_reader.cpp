//
// Created by Lauren on 19/11/26.
//

#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/ogg_reader.hpp>
#include <rtc_base/logging.h>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

namespace ntgcalls::io {
    OggReader::OggReader(media::BaseSink* sink): BaseIO(sink), ThreadedReader(sink) {}

    std::vector<ThreadedReader::Chunk> OggReader::read_frames(const std::chrono::nanoseconds duration) {
        std::vector<Chunk> frames;
        std::chrono::nanoseconds total{0};
        while (total < duration) {
            auto packet = next_packet();
            if (!packet) {
                break;
            }
            auto& [data, size] = *packet;
            const auto packet_duration = std::chrono::milliseconds(10) * std::max<size_t>(wrtc::audio_factory::OpusPacket::frames(data.get(), size), 1);
            frames.push_back({std::move(data), size, packet_duration, {0, webrtc::kVideoRotation_0, 0, 0, wrtc::models::Codec::Opus, true}});
            total += packet_duration;
        }
        if (frames.empty()) {
            RTC_LOG(LS_WARNING) << "Reached end of the Ogg stream";
            throw EOFError("Reached end of the Ogg stream");
        }
        return frames;
    }

    std::optional<std::pair<bytes::unique_binary, size_t>> OggReader::next_packet() {
        while (true) {
            if (auto packet = demuxer_.next()) {
                return packet;
            }
            if (eof_) {
                return std::nullopt;
            }
            try {
                const auto size = demuxer_.missing();
                const auto data = read(size);
                demuxer_.feed(data.get(), size);
            } catch (const EOFError&) {
                eof_ = true;
            }
        }
    }
} // ntgcalls::io
