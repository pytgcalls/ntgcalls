//
// Created by Lauren on 07/10/24.
//

#include <ntgcalls/io/audio_file_writer.hpp>
#include <ntgcalls/io/audio_shell_writer.hpp>
#include <ntgcalls/io/file_reader.hpp>
#include <ntgcalls/io/shell_reader.hpp>
#include <ntgcalls/media/media_source_factory.hpp>
#include <ntgcalls/media/devices/media_device.hpp>
#include <rtc_base/logging.h>

#define BOOST_THROW                                                                      \
    RTC_LOG(LS_ERROR) << "Shell execution is not yet supported on your OS/Architecture"; \
    throw ShellError("Shell execution is not yet supported on your OS/Architecture");

namespace ntgcalls::media {

    std::unique_ptr<io::BaseReader> MediaSourceFactory::from_input(const BaseMediaDescription& desc, BaseSink* sink) {
        if (const auto* video = dynamic_cast<const VideoDescription*>(&desc)) {
            if (video->width <= 0 || video->height <= 0 || video->fps == 0) {
                RTC_LOG(LS_ERROR) << "Invalid video resolution or fps";
                throw InvalidParams("Invalid video resolution or fps");
            }
        }
        // SUPPORTED INPUT MODES
        switch (desc.media_source) {
        case BaseMediaDescription::MediaSource::File:
            RTC_LOG(LS_INFO) << "Using file reader for " << desc.input;
            return std::make_unique<io::FileReader>(desc.input, sink);
        case BaseMediaDescription::MediaSource::Shell:
#ifdef BOOST_ENABLED
            RTC_LOG(LS_INFO) << "Using shell reader for " << desc.input;
            return std::make_unique<io::ShellReader>(desc.input, sink);
#else
            BOOST_THROW
#endif
        case BaseMediaDescription::MediaSource::Device:
            if (const auto* video = dynamic_cast<const VideoDescription*>(&desc)) {
                return devices::MediaDevice::create_camera_capture(*video, sink);
            }
            return devices::MediaDevice::create_device<io::BaseReader>(desc, sink, true);
        case BaseMediaDescription::MediaSource::FFmpeg:
            RTC_LOG(LS_ERROR) << "FFmpeg encoder is not yet supported";
            throw FFmpegError("FFmpeg encoder is not yet supported");
        case BaseMediaDescription::MediaSource::Desktop:
            if (const auto* video = dynamic_cast<const VideoDescription*>(&desc)) {
                return devices::MediaDevice::create_desktop_capture(*video, sink);
            }
            throw InvalidParams("Invalid media type");
        default:
            RTC_LOG(LS_ERROR) << "Invalid input mode";
            throw InvalidParams("Invalid input mode");
        }
    }

    std::unique_ptr<io::AudioWriter> MediaSourceFactory::from_audio_output(const BaseMediaDescription& desc, BaseSink* sink) {
        // SUPPORTED OUTPUT AUDIO MODES
        switch (desc.media_source) {
        case BaseMediaDescription::MediaSource::File:
            RTC_LOG(LS_INFO) << "Using file writer for " << desc.input;
            return std::make_unique<io::AudioFileWriter>(desc.input, sink);
        case BaseMediaDescription::MediaSource::Shell:
#ifdef BOOST_ENABLED
            RTC_LOG(LS_INFO) << "Using shell writer for " << desc.input;
            return std::make_unique<io::AudioShellWriter>(desc.input, sink);
#else
            BOOST_THROW
#endif
        case BaseMediaDescription::MediaSource::Device:
            return devices::MediaDevice::create_device<io::AudioWriter>(desc, sink, false);
        case BaseMediaDescription::MediaSource::FFmpeg:
            RTC_LOG(LS_ERROR) << "FFmpeg encoder is not yet supported";
            throw FFmpegError("FFmpeg encoder is not yet supported");
        default:
            RTC_LOG(LS_ERROR) << "Invalid input mode";
            throw InvalidParams("Invalid input mode");
        }
    }

} // ntgcalls::media
