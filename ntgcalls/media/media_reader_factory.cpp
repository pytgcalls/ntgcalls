//
// Created by Laky64 on 23/08/2023.
//

#include "media_reader_factory.hpp"

#include "ntgcalls/io/file_reader.hpp"
#include "ntgcalls/io/shell_reader.hpp"

namespace ntgcalls {
    MediaReaderFactory::MediaReaderFactory(const MediaDescription& desc, const int64_t audioSize, const int64_t videoSize) {
        if (desc.audio) {
            audio = fromInput(desc.audio.value(), audioSize);
            audio->start();
        }
        if (desc.video) {
            video = fromInput(desc.video.value(), videoSize);
            video->start();
        }
    }

    std::shared_ptr<BaseReader> MediaReaderFactory::fromInput(const BaseMediaDescription& desc, const int64_t bufferSize) {
        constexpr auto allowedFlags = BaseMediaDescription::InputMode::NoLatency;
        bool noLatency = desc.inputMode & BaseMediaDescription::InputMode::NoLatency;
        // SUPPORTED ENCODERS
        if ((desc.inputMode & (BaseMediaDescription::InputMode::File | allowedFlags)) == desc.inputMode) {
            RTC_LOG(LS_INFO) << "Using file reader for " << desc.input;
            return std::make_unique<FileReader>(desc.input, bufferSize, noLatency);
        }
        if ((desc.inputMode & (BaseMediaDescription::InputMode::Shell | allowedFlags)) == desc.inputMode) {
#ifdef BOOST_ENABLED
            RTC_LOG(LS_INFO) << "Using shell reader for " << desc.input;
            return std::make_unique<ShellReader>(desc.input, bufferSize, noLatency);
#else
            RTC_LOG(LS_ERROR) << "Shell execution is not yet supported on your OS/Architecture";
            throw ShellError("Shell execution is not yet supported on your OS/Architecture");
#endif
        }
        if ((desc.inputMode & (BaseMediaDescription::InputMode::FFmpeg | allowedFlags)) == desc.inputMode) {
            RTC_LOG(LS_ERROR) << "FFmpeg encoder is not yet supported";
            throw FFmpegError("FFmpeg encoder is not yet supported");
        }
        RTC_LOG(LS_ERROR) << "Encoder not found";
        throw InvalidParams("Encoder not found");
    }

    MediaReaderFactory::~MediaReaderFactory() {
        if (audio) {
            audio->close();
        }
        if (video) {
            video->close();
        }
        audio = nullptr;
        video = nullptr;
    }

} // ntgcalls