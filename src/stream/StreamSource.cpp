//
// Created by Laky64 on 02/08/2023.
//

#include "StreamSource.hpp"

StreamSource::StreamSource(bool isVideo) {
    this -> isVideo = isVideo;
    desc = std::make_shared<MediaDescription>(isVideo ? 1:0);
}
