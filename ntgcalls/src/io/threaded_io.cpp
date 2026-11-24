//
// Created by Lauren on 20/11/26.
//

#include <ntgcalls/io/threaded_io.hpp>

namespace ntgcalls::io {
    ThreadedIO::ThreadedIO(media::BaseSink* sink): BaseIO(sink) {}

    void ThreadedIO::spawn(std::function<void()> loop, const std::string& name) {
        threads_.push_back(webrtc::PlatformThread::SpawnJoinable(std::move(loop), name, webrtc::ThreadAttributes().SetPriority(webrtc::ThreadPriority::kRealtime)));
    }

    void ThreadedIO::join() {
        for (auto& thread : threads_) {
            thread.Finalize();
        }
        threads_.clear();
    }
} // ntgcalls::io
