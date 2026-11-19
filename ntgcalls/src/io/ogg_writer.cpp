//
// Created by Lauren on 20/11/26.
//

#include <array>
#include <ranges>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/ogg_writer.hpp>
#include <ntgcalls/media/audio_sink.hpp>
#include <rtc_base/logging.h>
#include <third_party/opus/src/include/opus.h>
#include <wrtc/audio_factory/opus/opus_packet.hpp>

namespace ntgcalls::io {
    OggWriter::OggWriter(media::BaseSink* sink): BaseIO(sink), ThreadedWriter(sink) {}

    OggWriter::~OggWriter() {
        for (const auto& decoder : decoders_ | std::views::values) {
            opus_decoder_destroy(decoder);
        }
        if (encoder_) {
            opus_encoder_destroy(encoder_);
        }
    }

    uint8_t OggWriter::channels() const {
        const auto audio_sink = dynamic_cast<media::AudioSink*>(sink_);
        const auto description = audio_sink ? audio_sink->get_config() : std::nullopt;
        return description ? description->channel_count : 2;
    }

    void OggWriter::send_frames(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) {
        const std::lock_guard lock(mutex_);
        if (!muxer_) {
            muxer_ = std::make_unique<media::containers::OggMuxer>(channels());
        }
        elapsed_ += wrtc::audio_factory::OpusPacket::kFrameSamples;
        if (!transcoding_) {
            for (const auto& ssrc : frames | std::views::keys) {
                if (!source_ssrc_) {
                    source_ssrc_ = ssrc;
                } else if (*source_ssrc_ != ssrc) {
                    RTC_LOG(LS_INFO) << "More than one participant, re-encoding the Opus recording";
                    transcoding_ = true;
                    break;
                }
            }
        }
        if (transcoding_) {
            transcode(frames);
        } else {
            forward(frames);
        }
    }

    void OggWriter::forward(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) {
        for (const auto& [data, size] : frames | std::views::values) {
            if (const int samples = wrtc::audio_factory::OpusPacket::samples(data.get(), size); samples > 0) {
                write_packet({data.get(), data.get() + size}, samples);
            }
        }
        while (muxer_->granule() + kPacketSamples * 2 <= elapsed_) {
            ensure_encoder();
            write_packet(silence_packet_, kPacketSamples);
        }
    }

    void OggWriter::transcode(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) {
        ensure_encoder();
        const auto count = channels();
        for (const auto& [ssrc, frame] : frames) {
            auto& decoder = decoders_[ssrc];
            if (!decoder) {
                int error = 0;
                decoder = opus_decoder_create(wrtc::audio_factory::OpusPacket::kSampleRate, count, &error);
                if (error != OPUS_OK) {
                    decoder = nullptr;
                    continue;
                }
            }
            std::vector<int16_t> pcm(static_cast<size_t>(kMaxPacketSamples) * count);
            const int samples = opus_decode(decoder, frame.first.get(), static_cast<opus_int32>(frame.second), pcm.data(), kMaxPacketSamples, 0);
            if (samples > 0) {
                auto& buffer = decoded_[ssrc];
                buffer.insert(buffer.end(), pcm.begin(), pcm.begin() + static_cast<std::ptrdiff_t>(samples) * count);
            }
        }
        const size_t block = static_cast<size_t>(wrtc::audio_factory::OpusPacket::kFrameSamples) * count;
        std::vector<int32_t> mix(block, 0);
        for (auto& buffer : decoded_ | std::views::values) {
            if (buffer.size() < block) {
                continue;
            }
            for (size_t i = 0; i < block; i++) {
                mix[i] += buffer[i];
            }
            buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(block));
        }
        for (const auto sample : mix) {
            mixed_.push_back(static_cast<int16_t>(std::clamp(sample, -32768, 32767)));
        }
        const size_t packet_block = static_cast<size_t>(kPacketSamples) * count;
        while (mixed_.size() >= packet_block) {
            std::array<uint8_t, 4000> packet{};
            const int size = opus_encode(encoder_, mixed_.data(), kPacketSamples, packet.data(), static_cast<opus_int32>(packet.size()));
            mixed_.erase(mixed_.begin(), mixed_.begin() + static_cast<std::ptrdiff_t>(packet_block));
            if (size > 0) {
                write_packet({packet.data(), packet.data() + size}, kPacketSamples);
            }
        }
    }

    void OggWriter::ensure_encoder() {
        if (encoder_) {
            return;
        }
        const auto count = channels();
        int error = 0;
        encoder_ = opus_encoder_create(wrtc::audio_factory::OpusPacket::kSampleRate, count, OPUS_APPLICATION_AUDIO, &error);
        if (error != OPUS_OK) {
            encoder_ = nullptr;
            throw FileError("Unable to create the Opus encoder");
        }
        const auto audio_sink = dynamic_cast<media::AudioSink*>(sink_);
        if (const auto description = audio_sink ? audio_sink->get_config() : std::nullopt; description && description->bitrate) {
            opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(static_cast<opus_int32>(*description->bitrate)));
        }
        const std::vector<int16_t> silence(static_cast<size_t>(kPacketSamples) * count, 0);
        std::array<uint8_t, 4000> packet{};
        const int size = opus_encode(encoder_, silence.data(), kPacketSamples, packet.data(), static_cast<opus_int32>(packet.size()));
        silence_packet_.assign(packet.data(), packet.data() + std::max(size, 0));
    }

    void OggWriter::write_packet(bytes::binary packet, const int samples) {
        push_pages(muxer_->add(std::move(packet), samples));
    }

    void OggWriter::push_pages(const std::vector<bytes::binary>& pages) {
        for (auto& page : pages) {
            auto data = bytes::make_unique_binary(page.size());
            std::memcpy(data.get(), page.data(), page.size());
            push(std::move(data), page.size());
        }
    }

    void OggWriter::flush() {
        const std::lock_guard lock(mutex_);
        if (muxer_) {
            push_pages(muxer_->finish());
        }
    }
} // ntgcalls::io
