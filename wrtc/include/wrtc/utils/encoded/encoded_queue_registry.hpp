//
// Created by Lauren on 20/11/26.
//

#pragma once
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <wrtc/utils/encoded/encoded_queue.hpp>

namespace wrtc::utils::encoded {
    class EncodedQueueRegistry {
        std::mutex mutex_;
        std::map<uint64_t, std::weak_ptr<EncodedQueue>> queues_;
        uint64_t next_id_ = 1;

    protected:
        std::shared_ptr<EncodedQueue> find_queue(const std::map<std::string, std::string>& parameters);

    public:
        static constexpr auto kPassthroughParam = "x-ntg-passthrough";

        uint64_t register_queue(const std::shared_ptr<EncodedQueue>& queue);

        void unregister_queue(uint64_t id);
    };
} // wrtc::utils::encoded
