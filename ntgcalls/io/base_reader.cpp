//
// Created by Laky64 on 04/08/2023.
//

#include "base_reader.hpp"


namespace ntgcalls {
    BaseReader::~BaseReader() {
        close();
    }

    void BaseReader::close() {
        readChunks = 0;
    }
}
