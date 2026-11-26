//
// Created by Lauren on 07/10/24.
//

#include <ntgcalls/io/container_reader.hpp>
#include <ntgcalls/io/container_writer.hpp>
#include <ntgcalls/io/file_reader.hpp>
#include <ntgcalls/io/file_writer.hpp>
#include <ntgcalls/io/raw_reader.hpp>
#include <ntgcalls/io/raw_writer.hpp>
#include <ntgcalls/io/shell_reader.hpp>
#include <ntgcalls/io/shell_writer.hpp>
#include <ntgcalls/media/containers/matroska/matroska_demuxer.hpp>
#include <ntgcalls/media/containers/matroska/matroska_muxer.hpp>
#include <ntgcalls/media/containers/ogg/ogg_demuxer.hpp>
#include <ntgcalls/media/containers/ogg/ogg_muxer.hpp>
#include <ntgcalls/media/media_source_factory.hpp>
#include <ntgcalls/media/devices/media_device.hpp>
#include <ntgcalls/media/recorders/audio_recorder.hpp>
#include <ntgcalls/media/recorders/video_recorder.hpp>
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
            switch (resolve_format(desc)) {
            case BaseMediaDescription::Format::Matroska:
                return std::make_unique<io::FileReader<io::ContainerReader<containers::MatroskaDemuxer>>>(desc.input, sink);
            case BaseMediaDescription::Format::Ogg:
                return std::make_unique<io::FileReader<io::ContainerReader<containers::OggDemuxer>>>(desc.input, sink);
            default:
                return std::make_unique<io::FileReader<io::RawReader>>(desc.input, sink);
            }
        case BaseMediaDescription::MediaSource::Shell:
#ifdef BOOST_ENABLED
            RTC_LOG(LS_INFO) << "Using shell reader for " << desc.input;
            switch (resolve_format(desc)) {
            case BaseMediaDescription::Format::Matroska:
                return std::make_unique<io::ShellReader<io::ContainerReader<containers::MatroskaDemuxer>>>(desc.input, sink);
            case BaseMediaDescription::Format::Ogg:
                return std::make_unique<io::ShellReader<io::ContainerReader<containers::OggDemuxer>>>(desc.input, sink);
            default:
                return std::make_unique<io::ShellReader<io::RawReader>>(desc.input, sink);
            }
#else
            BOOST_THROW
#endif
        case BaseMediaDescription::MediaSource::Device:
            if (const auto* video = dynamic_cast<const VideoDescription*>(&desc)) {
                return devices::MediaDevice::create_camera_capture(*video, sink);
            }
            return devices::MediaDevice::create_device<io::BaseReader>(desc, sink, true);
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

    BaseMediaDescription::Format MediaSourceFactory::resolve_format(const BaseMediaDescription& desc) {
        if (const auto* audio = dynamic_cast<const AudioDescription*>(&desc)) {
            if (const auto format = audio->format; format && *format != BaseMediaDescription::Format::Ogg && *format != BaseMediaDescription::Format::PCM) {
                RTC_LOG(LS_ERROR) << "Invalid audio format";
                throw InvalidParams("Audio supports only the Ogg and PCM formats");
            }
        }
        if (const auto* video = dynamic_cast<const VideoDescription*>(&desc)) {
            if (const auto format = video->format; format && *format != BaseMediaDescription::Format::Matroska && *format != BaseMediaDescription::Format::I420) {
                RTC_LOG(LS_ERROR) << "Invalid video format";
                throw InvalidParams("Video supports only the Matroska and I420 formats");
            }
        }
        return desc.effective_format();
    }

    std::unique_ptr<io::BaseWriter> MediaSourceFactory::from_output(const BaseMediaDescription& desc, BaseSink* sink) {
        const auto* video = dynamic_cast<const VideoDescription*>(&desc);
        if (video && (video->width <= 0 || video->height <= 0 || video->fps == 0)) {
            RTC_LOG(LS_ERROR) << "Invalid video resolution or fps";
            throw InvalidParams("Invalid video resolution or fps");
        }
        // SUPPORTED OUTPUT MODES
        switch (desc.media_source) {
        case BaseMediaDescription::MediaSource::File:
            RTC_LOG(LS_INFO) << "Using file writer for " << desc.input;
            switch (resolve_format(desc)) {
            case BaseMediaDescription::Format::Matroska:
                return std::make_unique<io::FileWriter<io::ContainerWriter<recorders::VideoRecorder, containers::MatroskaMuxer>>>(desc.input, sink);
            case BaseMediaDescription::Format::Ogg:
                return std::make_unique<io::FileWriter<io::ContainerWriter<recorders::AudioRecorder, containers::OggMuxer>>>(desc.input, sink);
            default:
                return std::make_unique<io::FileWriter<io::RawWriter>>(desc.input, sink);
            }
        case BaseMediaDescription::MediaSource::Shell:
#ifdef BOOST_ENABLED
            RTC_LOG(LS_INFO) << "Using shell writer for " << desc.input;
            switch (resolve_format(desc)) {
            case BaseMediaDescription::Format::Matroska:
                return std::make_unique<io::ShellWriter<io::ContainerWriter<recorders::VideoRecorder, containers::MatroskaMuxer>>>(desc.input, sink);
            case BaseMediaDescription::Format::Ogg:
                return std::make_unique<io::ShellWriter<io::ContainerWriter<recorders::AudioRecorder, containers::OggMuxer>>>(desc.input, sink);
            default:
                return std::make_unique<io::ShellWriter<io::RawWriter>>(desc.input, sink);
            }
#else
            BOOST_THROW
#endif
        case BaseMediaDescription::MediaSource::Device:
            if (video) {
                throw InvalidParams("Video playback to a device is not supported");
            }
            return devices::MediaDevice::create_device<io::BaseWriter>(desc, sink, false);
        default:
            RTC_LOG(LS_ERROR) << "Invalid input mode";
            throw InvalidParams("Invalid input mode");
        }
    }

} // ntgcalls::media
