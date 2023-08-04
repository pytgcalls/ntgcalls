//
// Created by Laky64 on 04/08/2023.
//

#ifndef NTGCALLS_BASEREADER_HPP
#define NTGCALLS_BASEREADER_HPP


#include <cstdint>
#include "rtc/rtc.hpp"

class BaseReader {
protected:
    int64_t chunkSize = 0, readChunks = 0;


public:
    virtual rtc::binary read() = 0;

    virtual ~BaseReader();

    virtual void close();
};


#endif //NTGCALLS_BASEREADER_HPP
