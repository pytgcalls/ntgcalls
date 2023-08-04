//
// Created by Laky64 on 04/08/2023.
//

#include "BaseReader.hpp"

BaseReader::~BaseReader() {
    close();
}

void BaseReader::close() {
    chunkSize = 0;
    readChunks = 0;
}
