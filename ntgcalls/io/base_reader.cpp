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
        close();
        readChunks = 0;
    }

    wrtc::binary BaseReader::read(size_t size) {
        if (dispatchQueue != nullptr) {
            wrtc::binary res;
            auto promise = std::make_shared<std::promise<void>>();
            if (!_eof && nextBuffer.size() <= 4) {
                dispatchQueue->dispatch([this, promise, size] {
                    try {
                        auto availableSpace = 10 - nextBuffer.size();
                        for (int i = 0; i < availableSpace; i++) {
                            auto tmpRead = readInternal(size);
                            if (tmpRead != nullptr) nextBuffer.push_back(tmpRead);
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
            if (!nextBuffer.empty()) {
                res = nextBuffer[0];
                nextBuffer.erase(nextBuffer.begin());
                return res;
            }
        }
        return nullptr;
    }

    void BaseReader::close() {
        dispatchQueue = nullptr;
    }

    bool BaseReader::eof() {
        return _eof && nextBuffer.empty();
    }
}
