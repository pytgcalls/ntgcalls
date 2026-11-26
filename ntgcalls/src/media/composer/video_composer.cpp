//
// Created by Lauren on 20/11/26.
//

#include <algorithm>
#include <cmath>
#include <ranges>
#include <libyuv/rotate.h>
#include <libyuv/scale.h>
#include <ntgcalls/media/composer/video_composer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::media::composer {
    VideoComposer::VideoComposer(const uint16_t width, const uint16_t height): width_(width & ~1), height_(height & ~1) {}

    size_t VideoComposer::frame_size() const {
        return static_cast<size_t>(width_) * height_ * 3 / 2;
    }

    void VideoComposer::update(const int64_t ssrc, bytes::binary frame, const uint16_t width, const uint16_t height, const webrtc::VideoRotation rotation) {
        if (width == 0 || height == 0 || frame.size() < static_cast<size_t>(width) * height * 3 / 2) {
            return;
        }
        const bool transposed = rotation == webrtc::kVideoRotation_90 || rotation == webrtc::kVideoRotation_270;
        if (rotation != webrtc::kVideoRotation_0) {
            frame = rotate(frame, width, height, rotation);
        }
        const std::lock_guard lock(mutex_);
        const auto now = Clock::now();
        auto& tile = tiles_[ssrc];
        const bool appeared = !tile.placed || tile.leaving;
        if (!tile.placed) {
            RTC_LOG(LS_INFO) << "Composing video " << ssrc << " " << width << "x" << height << " rotation " << static_cast<int>(rotation);
        }
        tile.frame = std::move(frame);
        tile.width = transposed ? height : width;
        tile.height = transposed ? width : height;
        tile.last_seen = now;
        tile.removed_at = std::nullopt;
        tile.leaving = false;
        if (appeared) {
            relayout(now);
        }
    }

    void VideoComposer::remove(const int64_t ssrc) {
        const std::lock_guard lock(mutex_);
        if (const auto it = tiles_.find(ssrc); it != tiles_.end() && !it->second.leaving) {
            it->second.removed_at = Clock::now();
        }
    }

    bytes::unique_binary VideoComposer::compose() {
        const std::lock_guard lock(mutex_);
        const auto now = Clock::now();
        if (expire(now)) {
            relayout(now);
        }
        const auto luma_size = static_cast<size_t>(width_) * height_;
        auto output = bytes::make_unique_binary(frame_size());
        std::memset(output.get(), 0, luma_size);
        std::memset(output.get() + luma_size, 128, luma_size / 2);
        for (const bool leaving : {true, false}) {
            for (const auto& tile : tiles_ | std::views::values) {
                if (tile.leaving == leaving) {
                    draw(output.get(), tile, current(tile, now));
                }
            }
        }
        return output;
    }

    bool VideoComposer::expire(const Clock::time_point now) {
        bool changed = false;
        for (auto it = tiles_.begin(); it != tiles_.end();) {
            auto& tile = it->second;
            if (tile.leaving && now - tile.start >= kAnimationTime) {
                it = tiles_.erase(it);
                continue;
            }
            if (!tile.leaving && tile.removed_at && now - *tile.removed_at >= kRemovalGrace) {
                tile.leaving = true;
                changed = true;
            }
            ++it;
        }
        return changed;
    }

    void VideoComposer::relayout(const Clock::time_point now) {
        size_t count = 0;
        for (const auto& tile : tiles_ | std::views::values) {
            count += tile.leaving ? 0 : 1;
        }
        size_t index = 0;
        for (auto& tile : tiles_ | std::views::values) {
            const auto position = current(tile, now);
            if (tile.leaving) {
                tile.from = position;
                tile.to = collapsed(position);
            } else {
                const auto target = cell(index++, count);
                tile.from = tile.placed ? position : collapsed(target);
                tile.to = target;
                tile.placed = true;
            }
            tile.start = now;
        }
    }

    VideoComposer::Rect VideoComposer::cell(const size_t index, const size_t count) const {
        const auto columns = static_cast<size_t>(std::ceil(std::sqrt(static_cast<double>(count))));
        const auto rows = (count + columns - 1) / columns;
        const double cell_width = static_cast<double>(width_) / static_cast<double>(columns);
        const double cell_height = static_cast<double>(height_) / static_cast<double>(rows);
        const auto row = index / columns;
        const auto column = index % columns;
        const auto in_row = row == rows - 1 ? count - row * columns : columns;
        const double offset = static_cast<double>(columns - in_row) * cell_width / 2;
        return {offset + static_cast<double>(column) * cell_width, static_cast<double>(row) * cell_height, cell_width, cell_height};
    }

    VideoComposer::Rect VideoComposer::current(const Tile& tile, const Clock::time_point now) {
        const auto elapsed = std::chrono::duration<double>(now - tile.start).count();
        const auto progress = ease(std::clamp(elapsed / std::chrono::duration<double>(kAnimationTime).count(), 0.0, 1.0));
        return {
            tile.from.x + (tile.to.x - tile.from.x) * progress,
            tile.from.y + (tile.to.y - tile.from.y) * progress,
            tile.from.width + (tile.to.width - tile.from.width) * progress,
            tile.from.height + (tile.to.height - tile.from.height) * progress,
        };
    }

    VideoComposer::Rect VideoComposer::collapsed(const Rect& rect) {
        return {rect.x + rect.width / 2, rect.y + rect.height / 2, 0, 0};
    }

    double VideoComposer::ease(const double progress) {
        return progress < 0.5 ? 4 * progress * progress * progress : 1 - std::pow(-2 * progress + 2, 3) / 2;
    }

    int VideoComposer::even(const double value) {
        return static_cast<int>(std::lround(value / 2)) * 2;
    }

    bytes::binary VideoComposer::rotate(const bytes::binary& frame, const uint16_t width, const uint16_t height, const webrtc::VideoRotation rotation) {
        const bool transposed = rotation == webrtc::kVideoRotation_90 || rotation == webrtc::kVideoRotation_270;
        const int output_width = transposed ? height : width;
        const auto luma_size = static_cast<size_t>(width) * height;
        const auto chroma_width = (width + 1) / 2;
        const auto chroma_size = static_cast<size_t>(chroma_width) * ((height + 1) / 2);
        const auto output_chroma_width = (output_width + 1) / 2;
        bytes::binary output(frame.size());
        libyuv::I420Rotate(
            frame.data(), width,
            frame.data() + luma_size, chroma_width,
            frame.data() + luma_size + chroma_size, chroma_width,
            output.data(), output_width,
            output.data() + luma_size, output_chroma_width,
            output.data() + luma_size + chroma_size, output_chroma_width,
            width, height,
            static_cast<libyuv::RotationMode>(rotation)
        );
        return output;
    }

    void VideoComposer::draw(uint8_t* output, const Tile& tile, const Rect& area) const {
        const double scale = std::min(area.width / tile.width, area.height / tile.height);
        const double fitted_width = tile.width * scale;
        const double fitted_height = tile.height * scale;
        const int x = std::clamp(even(area.x + (area.width - fitted_width) / 2), 0, static_cast<int>(width_));
        const int y = std::clamp(even(area.y + (area.height - fitted_height) / 2), 0, static_cast<int>(height_));
        const int width = std::min(even(fitted_width), static_cast<int>(width_) - x);
        const int height = std::min(even(fitted_height), static_cast<int>(height_) - y);
        if (width < 2 || height < 2) {
            return;
        }
        const auto source_luma = static_cast<size_t>(tile.width) * tile.height;
        const auto source_chroma_width = (tile.width + 1) / 2;
        const auto source_chroma = static_cast<size_t>(source_chroma_width) * ((tile.height + 1) / 2);
        const auto* source = tile.frame.data();
        const auto output_luma = static_cast<size_t>(width_) * height_;
        const auto output_chroma_width = width_ / 2;
        auto* luma = output + static_cast<size_t>(y) * width_ + x;
        auto* chroma_u = output + output_luma + static_cast<size_t>(y / 2) * output_chroma_width + x / 2;
        auto* chroma_v = chroma_u + output_luma / 4;
        libyuv::I420Scale(
            source, tile.width,
            source + source_luma, source_chroma_width,
            source + source_luma + source_chroma, source_chroma_width,
            tile.width, tile.height,
            luma, width_,
            chroma_u, output_chroma_width,
            chroma_v, output_chroma_width,
            width, height,
            libyuv::kFilterBox
        );
    }
} // ntgcalls::media::composer
