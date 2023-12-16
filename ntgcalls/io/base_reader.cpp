//
// Created by Laky64 on 04/08/2023.
//

#include "base_reader.hpp"
#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {
    BaseReader::BaseReader() {
        dispatchQueue = std::make_shared<DispatchQueue>();
    }

    BaseReader::~BaseReader() {
        BaseReader::close();
        readChunks = 0;
    }

    wrtc::binary BaseReader::read(int64_t size) {
        if (dispatchQueue != nullptr) {
            auto promise = std::make_shared<std::promise<void>>();
            if (!_eof && nextBuffer.size() <= 4) {
                dispatchQueue->dispatch([this, promise, size] {
                    try {
                        const auto availableSpace = 10 - nextBuffer.size();
                        for (int i = 0; i < availableSpace; i++) {
                            std::lock_guard lock(mutex);
                            if (auto tmpRead = readInternal(size); tmpRead != nullptr) nextBuffer.push_back(tmpRead);
                        }
                    } catch (...) {
                        _eof = true;
                    }
                    if (promise != nullptr) promise->set_value();
                });
            }
            if (nextBuffer.empty() && !_eof) {
                if (promise != nullptr) promise->get_future().wait();
            }
            std::lock_guard lock(mutex);
            if (!nextBuffer.empty()) {
                wrtc::binary res = nextBuffer[0];
                nextBuffer.erase(nextBuffer.begin());
                return res;
            }
        }
        return nullptr;
    }

    void BaseReader::close() {
        dispatchQueue = nullptr;
    }

    bool BaseReader::eof() const
    {
        return _eof && nextBuffer.empty();
    }
}
