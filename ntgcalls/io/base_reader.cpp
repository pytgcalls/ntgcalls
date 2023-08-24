//
// Created by Laky64 on 04/08/2023.
//

#include "base_reader.hpp"

namespace ntgcalls {

    wrtc::binary BaseReader::read(size_t size) {
        /*wrtc::binary res;
        promise = {};
        if (!_eof) {
            dispatchQueue.dispatch([this, size] {
                wrtc::binary tmpBuff = readInternal(size);
                mtx.lock();
                nextBuffer.push_back(tmpBuff);
                mtx.unlock();
                if (!eofInternal()) {
                    tmpBuff = readInternal(size);
                }
                mtx.lock();
                nextBuffer.push_back(tmpBuff);
                mtx.unlock();
                _eof = eofInternal();
                promise.set_value();
                delete[] tmpBuff;
            });
        }
        if (nextBuffer.empty() && !_eof) {
            promise.get_future().wait();
        }
        res = nextBuffer[0];
        mtx.lock();
        nextBuffer.erase(nextBuffer.begin(), nextBuffer.begin() + 1);
        mtx.unlock();
        return res;*/
        return readInternal(size);
    }

    void BaseReader::close() {
        readChunks = 0;
    }

    bool BaseReader::eof() {
        return _eof;
    }
}
