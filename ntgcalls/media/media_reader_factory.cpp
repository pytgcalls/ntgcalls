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
        switch (desc.inputMode) {
            case BaseMediaDescription::InputMode::File:
                return std::make_shared<FileReader>(desc.input);
            case BaseMediaDescription::InputMode::Shell:
#ifdef BOOST_ENABLED
                return std::make_shared<ShellReader>(desc.input);
#else
                throw ShellError("Shell execution is not yet supported on your OS/Architecture");
#endif
            case BaseMediaDescription::InputMode::FFmpeg:
                throw FFmpegError("FFmpeg encoder is not yet supported");
        }
        throw InvalidParams("Encoder not found");
    }

    MediaReaderFactory::~MediaReaderFactory() {
        audio = nullptr;
        video = nullptr;
    }

} // ntgcalls