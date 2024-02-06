//
// Created by Laky64 on 01/08/2023.
//

#pragma once


#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

class DispatchQueue {
    typedef std::function<void()> fp_t;

public:
    explicit DispatchQueue(size_t threadCount = 1);
    ~DispatchQueue();

    void dispatch(const fp_t& op);

    void dispatch(fp_t&& op);

private:
    std::mutex lockMutex;
    std::vector<std::thread> threads;
    std::queue<fp_t> queue;
    std::condition_variable condition;
    bool quit = false;

    void dispatchThreadHandler();
};
