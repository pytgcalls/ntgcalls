//
// Created by Lauren on 20/11/26.
//

#include <wrtc/utils/encoded/encoded_queue_registry.hpp>

namespace wrtc::utils::encoded {
    uint64_t EncodedQueueRegistry::register_queue(const std::shared_ptr<EncodedQueue>& queue) {
        const std::lock_guard lock(mutex_);
        const auto id = next_id_++;
        queues_[id] = queue;
        return id;
    }

    void EncodedQueueRegistry::unregister_queue(const uint64_t id) {
        const std::lock_guard lock(mutex_);
        queues_.erase(id);
    }

    std::shared_ptr<EncodedQueue> EncodedQueueRegistry::find_queue(const std::map<std::string, std::string>& parameters) {
        const auto param = parameters.find(kPassthroughParam);
        if (param == parameters.end()) {
            return nullptr;
        }
        const std::lock_guard lock(mutex_);
        if (const auto it = queues_.find(std::stoull(param->second)); it != queues_.end()) {
            return it->second.lock();
        }
        return nullptr;
    }
} // wrtc::utils::encoded
