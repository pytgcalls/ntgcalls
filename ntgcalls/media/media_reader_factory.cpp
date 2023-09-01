//
// Created by Laky64 on 23/08/2023.
//

#include "media_reader_factory.hpp"

namespace ntgcalls {
    MediaReaderFactory::MediaReaderFactory(MediaDescription desc) {
        if (desc.audio) {
            audio = fromInput(desc.audio.value());
        }
        if (desc.video) {
            video = fromInput(desc.video.value());
        }
    }

    std::shared_ptr<BaseReader> MediaReaderFactory::fromInput(BaseMediaDescription desc) {
        // SUPPORTED ENCODERS
        switch (desc.encoder) {
            case BaseMediaDescription::Encoder::Raw:
                return std::make_shared<FileReader>(desc.input);
            case BaseMediaDescription::Encoder::Shell:
                return std::make_shared<ShellReader>(desc.input);
            case BaseMediaDescription::Encoder::FFmpeg:
                throw FFmpegError("FFmpeg encoder is not yet supported");
        }
        throw InvalidParams("Encoder not found");
    }

    MediaReaderFactory::~MediaReaderFactory() {
        audio = nullptr;
        video = nullptr;
    }

} // ntgcalls