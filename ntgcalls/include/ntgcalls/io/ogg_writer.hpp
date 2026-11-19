//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <map>
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
        std::optional<int64_t> source_ssrc_;
        bool transcoding_ = false;
        std::map<int64_t, OpusDecoder*> decoders_;
        std::map<int64_t, std::vector<int16_t>> decoded_;
        OpusEncoder* encoder_ = nullptr;
        std::vector<int16_t> mixed_;
        bytes::binary silence_packet_;

        uint8_t channels() const;

        void ensure_encoder();

        void write_packet(bytes::binary packet, int samples);

        void push_pages(const std::vector<bytes::binary>& pages);

        void forward(const std::vector<wrtc::models::Frame>& frames);

        void transcode(const std::vector<wrtc::models::Frame>& frames);

    public:
        explicit OggWriter(media::BaseSink* sink);

        ~OggWriter() override;

        void send_frames(std::vector<wrtc::models::Frame> frames) override;

        void flush() override;
    };

} // ntgcalls::io
