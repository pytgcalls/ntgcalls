//
// Created by Laky64 on 04/08/2023.
//

#include "base_reader.hpp"

namespace ntgcalls {

    wrtc::binary BaseReader::read(size_t size) {
        wrtc::binary res;
        auto promise = std::make_shared<std::promise<void>>();
        if (!_eof && nextBuffer.size() <= 10) {
            dispatchQueue.dispatch([this, promise, size] {
                wrtc::binary tmpBuff = readInternal(size);
                mtx.lock();
                nextBuffer.push_back(tmpBuff);
                mtx.unlock();
                if (!eofInternal()) {
                    tmpBuff = readInternal(size);
                    mtx.lock();
                    nextBuffer.push_back(tmpBuff);
                    mtx.unlock();
                }
                _eof = eofInternal();
                promise->set_value();
            });
        }
        if (nextBuffer.empty() && !_eof) {
            promise->get_future().wait();
        }
        mtx.lock();
        res = nextBuffer[0];
        nextBuffer.erase(nextBuffer.begin(), nextBuffer.begin() + 1);
        mtx.unlock();
        return res;
    }

    void BaseReader::close() {
        readChunks = 0;
    }

    bool BaseReader::eof() {
        return _eof && nextBuffer.empty();
    }
}
