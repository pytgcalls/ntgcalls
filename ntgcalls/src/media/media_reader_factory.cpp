//
// Created by Laky64 on 23/08/2023.
//

#include <ntgcalls/media/media_reader_factory.hpp>

#include <ntgcalls/devices/media_device.hpp>
#include <ntgcalls/io/file_reader.hpp>
#include <ntgcalls/io/shell_reader.hpp>

namespace ntgcalls {

    std::unique_ptr<BaseReader> MediaReaderFactory::fromInput(const BaseMediaDescription& desc, BaseSink *sink) {
        // SUPPORTED INPUT MODES
        switch (desc.inputMode) {
        case BaseMediaDescription::InputMode::File:
            RTC_LOG(LS_INFO) << "Using file reader for " << desc.input;
            return std::make_unique<FileReader>(desc.input, sink);
        case BaseMediaDescription::InputMode::Shell:
#ifdef BOOST_ENABLED
            RTC_LOG(LS_INFO) << "Using shell reader for " << desc.input;
            return std::make_unique<ShellReader>(desc.input, sink);
#else
            RTC_LOG(LS_ERROR) << "Shell execution is not yet supported on your OS/Architecture";
            throw ShellError("Shell execution is not yet supported on your OS/Architecture");
#endif
        case BaseMediaDescription::InputMode::Device:
            return MediaDevice::CreateInput(desc, sink);
        case BaseMediaDescription::InputMode::FFmpeg:
            RTC_LOG(LS_ERROR) << "FFmpeg encoder is not yet supported";
            throw FFmpegError("FFmpeg encoder is not yet supported");
        default:
            RTC_LOG(LS_ERROR) << "Invalid input mode";
            throw InvalidParams("Invalid input mode");
        }
    }

} // ntgcalls