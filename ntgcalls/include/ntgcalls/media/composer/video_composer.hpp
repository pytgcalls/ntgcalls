//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <api/video/video_rotation.h>
#include <wrtc/utils/binary.hpp>

namespace ntgcalls::media::composer {

    class VideoComposer {
        using Clock = std::chrono::steady_clock;

        static constexpr auto kAnimationTime = std::chrono::milliseconds(300);
        static constexpr auto kRemovalGrace = std::chrono::milliseconds(1500);

        struct Rect {
            double x = 0;
            double y = 0;
            double width = 0;
            double height = 0;
        };

        struct Tile {
            bytes::binary frame;
            uint16_t width = 0;
            uint16_t height = 0;
            Rect from;
            Rect to;
            Clock::time_point start;
            Clock::time_point last_seen;
            std::optional<Clock::time_point> removed_at;
            bool placed = false;
            bool leaving = false;
        };

        uint16_t width_;
        uint16_t height_;
        std::map<int64_t, Tile> tiles_;
        std::mutex mutex_;

        void relayout(Clock::time_point now);

        bool expire(Clock::time_point now);

        void draw(uint8_t* output, const Tile& tile, const Rect& area) const;

        [[nodiscard]] Rect cell(size_t index, size_t count) const;

        static Rect current(const Tile& tile, Clock::time_point now);

        static Rect collapsed(const Rect& rect);

        static double ease(double progress);

        static int even(double value);

        static bytes::binary rotate(const bytes::binary& frame, uint16_t width, uint16_t height, webrtc::VideoRotation rotation);

    public:
        VideoComposer(uint16_t width, uint16_t height);

        void update(int64_t ssrc, bytes::binary frame, uint16_t width, uint16_t height, webrtc::VideoRotation rotation);

        void remove(int64_t ssrc);

        bytes::unique_binary compose();

        [[nodiscard]] size_t frame_size() const;
    };

} // ntgcalls::media::composer
