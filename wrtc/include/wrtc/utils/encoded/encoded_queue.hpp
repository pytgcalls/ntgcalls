//
// Created by Lauren on 19/11/26.
//

#pragma once
#include <deque>
#include <mutex>
#include <optional>
#include <wrtc/utils/encoded/encoded_frame.hpp>

namespace wrtc::utils::encoded {
    class EncodedQueue {
        std::mutex mutex_;
        std::deque<EncodedFrame> frames_;

    public:
        void push(EncodedFrame frame);

        void push_front(EncodedFrame frame);

        std::optional<EncodedFrame> pop();

        void clear();
    };
} // wrtc::utils::encoded
