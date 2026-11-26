//
// Created by Lauren on 26/11/26.
//

#pragma once

#include <atomic>
#include <cstring>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>
#include <ntgcalls/io/threaded_writer.hpp>
#include <ntgcalls/media/containers/muxer.hpp>
#include <ntgcalls/media/mixers/mixer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::io {

    template<std::derived_from<media::mixers::Mixer> Mixer, std::derived_from<media::containers::Muxer> Muxer>
    class ContainerWriter: public ThreadedWriter {
        static_assert(Muxer::supports(Mixer::kKind), "The container cannot hold this kind of media");

        std::mutex muxer_mutex_;
        std::optional<Muxer> muxer_;
        bool failed_ = false;
        std::atomic<std::thread::id> ticking_thread_;
        std::vector<media::containers::MuxedChunk> idle_chunks_;
        Mixer mixer_;

        void on_packet(const media::containers::TrackInfo& track, media::containers::EncodedPacket packet) {
            media::containers::MuxedChunk chunk;
            {
                const std::lock_guard lock(muxer_mutex_);
                if (failed_) {
                    return;
                }
                try {
                    if (!muxer_) {
                        muxer_.emplace(track);
                    }
                    chunk = muxer_->add(std::move(packet));
                } catch (const std::exception& e) {
                    stop_writing(e);
                    return;
                }
                if (chunk.size == 0) {
                    return;
                }
                if (ticking_thread_.load() == std::this_thread::get_id()) {
                    idle_chunks_.push_back(std::move(chunk));
                    return;
                }
            }
            push(std::move(chunk.data), chunk.size);
        }

        void stop_writing(const std::exception& e) {
            RTC_LOG(LS_ERROR) << "Stopping the recording: " << e.what();
            failed_ = true;
            muxer_.reset();
        }

        static std::optional<std::pair<bytes::unique_binary, size_t>> join(std::vector<media::containers::MuxedChunk> chunks) {
            if (chunks.empty()) {
                return std::nullopt;
            }
            if (chunks.size() == 1) {
                return std::pair{std::move(chunks.front().data), chunks.front().size};
            }
            size_t size = 0;
            for (const auto& chunk : chunks) {
                size += chunk.size;
            }
            auto data = bytes::make_unique_binary(size);
            size_t offset = 0;
            for (const auto& chunk : chunks) {
                std::memcpy(data.get() + offset, chunk.data.get(), chunk.size);
                offset += chunk.size;
            }
            return std::pair{std::move(data), size};
        }

    protected:
        std::optional<std::pair<bytes::unique_binary, size_t>> idle_data() override {
            ticking_thread_.store(std::this_thread::get_id());
            mixer_.tick();
            ticking_thread_.store(std::thread::id());
            std::vector<media::containers::MuxedChunk> chunks;
            {
                const std::lock_guard lock(muxer_mutex_);
                chunks.swap(idle_chunks_);
            }
            return join(std::move(chunks));
        }

        std::chrono::nanoseconds idle_interval() const override {
            return mixer_.tick_interval();
        }

    public:
        explicit ContainerWriter(media::BaseSink* sink):
        BaseIO(sink),
        ThreadedWriter(sink),
        mixer_(sink, static_cast<bool (*)(wrtc::models::Codec)>(&Muxer::supports), [this](const media::containers::TrackInfo& track, media::containers::EncodedPacket packet) {
            on_packet(track, std::move(packet));
        }) {}

        void send_frames(std::vector<wrtc::models::Frame> frames) override {
            try {
                mixer_.send_frames(std::move(frames));
            } catch (const std::exception& e) {
                const std::lock_guard lock(muxer_mutex_);
                stop_writing(e);
            }
        }

        void remove_source(const int64_t ssrc) override {
            mixer_.remove_source(ssrc);
        }

        void flush() override {
            try {
                mixer_.flush();
            } catch (const std::exception& e) {
                const std::lock_guard lock(muxer_mutex_);
                stop_writing(e);
            }
            media::containers::MuxedChunk chunk;
            {
                const std::lock_guard lock(muxer_mutex_);
                try {
                    if (muxer_) {
                        chunk = muxer_->finish();
                    }
                } catch (const std::exception& e) {
                    stop_writing(e);
                }
            }
            if (chunk.size > 0) {
                push(std::move(chunk.data), chunk.size);
            }
        }
    };

} // ntgcalls::io
