//
// Created by Laky64 on 19/03/2024.
//
#include "binding_utils.hpp"

std::mutex GlobalAsync::mutex{};
std::shared_ptr<DispatchQueue> GlobalAsync::dispatch_queue = nullptr;

std::shared_ptr<DispatchQueue> GlobalAsync::GetOrCreate() {
    std::lock_guard lock(mutex);
    if (!dispatch_queue) {
        dispatch_queue = std::make_shared<DispatchQueue>(10);
    }
    return dispatch_queue;
}
