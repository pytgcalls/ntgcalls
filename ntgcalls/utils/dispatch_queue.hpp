//
// Created by Laky64 on 01/08/2023.
//

#ifndef NTGCALLS_DISPATCHQUEUE_HPP
#define NTGCALLS_DISPATCHQUEUE_HPP


#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <string>

class DispatchQueue {
    typedef std::function<void(void)> fp_t;

public:
    explicit DispatchQueue(std::string name, size_t threadCount = 1);
    ~DispatchQueue();

    void dispatch(const fp_t& op);
    void dispatch(fp_t&& op);

    void removePending();

    DispatchQueue(const DispatchQueue& rhs) = delete;
    DispatchQueue& operator=(const DispatchQueue& rhs) = delete;
    DispatchQueue(DispatchQueue&& rhs) = delete;
    DispatchQueue& operator=(DispatchQueue&& rhs) = delete;

private:
    std::string name;
    std::mutex lockMutex;
    std::vector<std::thread> threads;
    std::queue<fp_t> queue;
    std::condition_variable condition;
    bool quit = false;

    void dispatchThreadHandler();
};


#endif
