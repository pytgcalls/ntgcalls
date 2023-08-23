//
// Created by Laky64 on 23/08/2023.
//

#include "media_reader_factory.hpp"

namespace ntgcalls {
    MediaReaderFactory::MediaReaderFactory(MediaDescription desc) {
        // SUPPORTED ENCODERS
        if (desc.encoder == "raw") {
            if (desc.audio) {
                audio = std::make_shared<FileReader>(desc.audio->path);
            }
            if (desc.video) {
                video = std::make_shared<FileReader>(desc.video->path);
            }
        } else if (desc.encoder == "ffmpeg") {
            throw FFmpegError("FFmpeg encoder is not yet supported");
        } else {
            throw InvalidParams("Encoder \"" + desc.encoder + "\" not found");
        }
    }

    MediaReaderFactory::~MediaReaderFactory() {
        audio = nullptr;
        video = nullptr;
    }

} // ntgcalls