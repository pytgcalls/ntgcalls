//
// Created by Lauren on 19/11/26.
//

#include <wrtc/utils/encoded/encoded_queue.hpp>

namespace wrtc::utils::encoded {
    void EncodedQueue::push(EncodedFrame frame) {
        const std::lock_guard lock(mutex_);
        frames_.push_back(std::move(frame));
    }

    void EncodedQueue::push_front(EncodedFrame frame) {
        const std::lock_guard lock(mutex_);
        frames_.push_front(std::move(frame));
    }

    std::optional<EncodedFrame> EncodedQueue::pop() {
        const std::lock_guard lock(mutex_);
        if (frames_.empty()) {
            return std::nullopt;
        }
        auto frame = std::move(frames_.front());
        frames_.pop_front();
        return frame;
    }

    void EncodedQueue::clear() {
        const std::lock_guard lock(mutex_);
        frames_.clear();
    }
} // wrtc::utils::encoded
