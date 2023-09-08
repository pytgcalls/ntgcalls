//
// Created by Laky64 on 04/08/2023.
//

#include "base_reader.hpp"
#include "ntgcalls/exceptions.hpp"

namespace ntgcalls {
    BaseReader::BaseReader() {
        dispatchQueue = std::make_shared<DispatchQueue>("Reader_" + rtc::CreateRandomUuid());
    }

    BaseReader::~BaseReader() {
        close();
        readChunks = 0;
    }

    wrtc::binary BaseReader::read(size_t size) {
        wrtc::binary res;
        auto promise = std::make_shared<std::promise<void>>();
        if (!_eof && nextBuffer.size() <= 10) {
            dispatchQueue->dispatch([this, promise, size] {
                try {
                    nextBuffer.push_back(readInternal(size));
                    nextBuffer.push_back(readInternal(size));
                } catch (...) {
                    _eof = true;
                }
                promise->set_value();
            });
        }
        if (nextBuffer.empty() && !_eof) {
            promise->get_future().wait();
        }
        res = nextBuffer[0];
        nextBuffer.erase(nextBuffer.begin(), nextBuffer.begin() + 1);
        return res;
    }

    void BaseReader::close() {
        dispatchQueue = nullptr;
    }

    bool BaseReader::eof() {
        return _eof && nextBuffer.empty();
    }
}
