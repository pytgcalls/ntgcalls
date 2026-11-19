//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <memory>
#include <ntgcalls/io/threaded_writer.hpp>
#include <ntgcalls/media/containers/ogg/ogg_muxer.hpp>

struct OpusDecoder;
struct OpusEncoder;

namespace ntgcalls::io {

    class OggWriter: public ThreadedWriter {
        static constexpr int kPacketSamples = 960;
        static constexpr int kMaxPacketSamples = 5760;

        std::mutex mutex_;
        std::unique_ptr<media::containers::OggMuxer> muxer_;
        uint64_t elapsed_ = 0;
        std::optional<uint32_t> source_ssrc_;
        bool transcoding_ = false;
        std::map<uint32_t, OpusDecoder*> decoders_;
        std::map<uint32_t, std::vector<int16_t>> decoded_;
        OpusEncoder* encoder_ = nullptr;
        std::vector<int16_t> mixed_;
        bytes::binary silence_packet_;

        uint8_t channels() const;

        void ensure_encoder();

        void write_packet(bytes::binary packet, int samples);

        void push_pages(const std::vector<bytes::binary>& pages);

        void forward(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames);

        void transcode(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames);

    public:
        explicit OggWriter(media::BaseSink* sink);

        ~OggWriter() override;

        void send_frames(const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) override;

        void flush() override;
    };

} // ntgcalls::io
