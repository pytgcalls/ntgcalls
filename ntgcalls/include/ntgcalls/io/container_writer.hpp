//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <cstring>
#include <mutex>
#include <optional>
#include <ntgcalls/io/threaded_writer.hpp>
#include <ntgcalls/media/containers/muxer.hpp>
#include <ntgcalls/media/recorders/recorder.hpp>

namespace ntgcalls::io {

    template<std::derived_from<media::recorders::Recorder> Recorder, std::derived_from<media::containers::Muxer> Muxer>
    class ContainerWriter: public ThreadedWriter {
        static_assert(Muxer::supports(Recorder::kKind), "The container cannot hold this kind of media");

        std::mutex muxer_mutex_;
        std::optional<Muxer> muxer_;
        Recorder recorder_;

        void on_packet(const media::containers::TrackInfo& track, const media::containers::EncodedPacket& packet) {
            bytes::binary output;
            {
                const std::lock_guard lock(muxer_mutex_);
                if (!muxer_) {
                    muxer_.emplace(track);
                }
                output = muxer_->add(packet);
            }
            push_output(output);
        }

        void push_output(const bytes::binary& output) {
            if (output.empty()) {
                return;
            }
            auto buffer = bytes::make_unique_binary(output.size());
            std::memcpy(buffer.get(), output.data(), output.size());
            push(std::move(buffer), output.size());
        }

    protected:
        std::optional<std::pair<bytes::unique_binary, size_t>> idle_data() override {
            recorder_.tick();
            return std::nullopt;
        }

        std::chrono::nanoseconds idle_interval() const override {
            return recorder_.tick_interval();
        }

    public:
        explicit ContainerWriter(media::BaseSink* sink):
        BaseIO(sink),
        ThreadedWriter(sink),
        recorder_(sink, static_cast<bool (*)(wrtc::models::Codec)>(&Muxer::supports), [this](const media::containers::TrackInfo& track, const media::containers::EncodedPacket& packet) {
            on_packet(track, packet);
        }) {}

        void send_frames(std::vector<wrtc::models::Frame> frames) override {
            recorder_.send_frames(std::move(frames));
        }

        void remove_source(const int64_t ssrc) override {
            recorder_.remove_source(ssrc);
        }

        void flush() override {
            recorder_.flush();
            bytes::binary output;
            {
                const std::lock_guard lock(muxer_mutex_);
                if (muxer_) {
                    output = muxer_->finish();
                }
            }
            push_output(output);
        }
    };

} // ntgcalls::io
