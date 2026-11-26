//
// Created by Lauren on 07/10/24.
//

#include <ntgcalls/io/container_reader.hpp>
#include <ntgcalls/io/container_writer.hpp>
#include <ntgcalls/io/file_io.hpp>
#include <ntgcalls/io/shell_io.hpp>
#include <ntgcalls/media/containers/libav/libav_demuxer.hpp>
#include <ntgcalls/media/containers/libav/libav_format.hpp>
#include <ntgcalls/media/containers/libav/libav_muxer.hpp>
#include <ntgcalls/media/containers/raw/raw_demuxer.hpp>
#include <ntgcalls/media/containers/raw/raw_muxer.hpp>
#include <ntgcalls/media/media_source_factory.hpp>
#include <ntgcalls/media/devices/media_device.hpp>
#include <ntgcalls/media/mixers/audio_mixer.hpp>
#include <ntgcalls/media/mixers/video_mixer.hpp>
#include <rtc_base/logging.h>

#define BOOST_THROW                                                                      \
    RTC_LOG(LS_ERROR) << "Shell execution is not yet supported on your OS/Architecture"; \
    throw ShellError("Shell execution is not yet supported on your OS/Architecture");

namespace ntgcalls::media {
    std::unique_ptr<io::BaseReader> MediaSourceFactory::from_input(const BaseMediaDescription& desc, BaseSink* sink) {
        if (const auto* video = dynamic_cast<const VideoDescription*>(&desc); video && (video->width <= 0 || video->height <= 0 || video->fps == 0)) {
            RTC_LOG(LS_ERROR) << "Invalid video resolution or fps";
            throw InvalidParams("Invalid video resolution or fps");
        }

        // SUPPORTED INPUT MODES
        switch (desc.media_source) {
        case BaseMediaDescription::MediaSource::File:
            RTC_LOG(LS_INFO) << "Using file reader for " << desc.input;
            return create_reader<io::FileIO>(desc, sink);
        case BaseMediaDescription::MediaSource::Shell:
#ifdef BOOST_ENABLED
            RTC_LOG(LS_INFO) << "Using shell reader for " << desc.input;
            return create_reader<io::ShellIO>(desc, sink);
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
            return create_writer<io::FileIO>(desc, sink);
        case BaseMediaDescription::MediaSource::Shell:
#ifdef BOOST_ENABLED
            RTC_LOG(LS_INFO) << "Using shell writer for " << desc.input;
            return create_writer<io::ShellIO>(desc, sink);
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

    template<template<typename> typename IO>
    std::unique_ptr<io::BaseReader> MediaSourceFactory::create_reader(const BaseMediaDescription& desc, BaseSink* sink) {
        if (const auto format = input_format(desc); format == BaseMediaDescription::Format::PCM || format == BaseMediaDescription::Format::I420) {
            return std::make_unique<IO<io::ContainerReader<containers::RawDemuxer>>>(desc.input, sink);
        }
        return std::make_unique<IO<io::ContainerReader<containers::LibAvDemuxer>>>(desc.input, sink);
    }

    template<template<typename> typename IO>
    std::unique_ptr<io::BaseWriter> MediaSourceFactory::create_writer(const BaseMediaDescription& desc, BaseSink* sink) {
        const auto format = output_format(desc);
        if (dynamic_cast<const VideoDescription*>(&desc)) {
            return create_mixer_writer<IO, mixers::VideoMixer>(format, desc, sink);
        }
        return create_mixer_writer<IO, mixers::AudioMixer>(format, desc, sink);
    }

    template<template<typename> typename IO, typename Mixer>
    std::unique_ptr<io::BaseWriter> MediaSourceFactory::create_mixer_writer(const BaseMediaDescription::Format format, const BaseMediaDescription& desc, BaseSink* sink) {
        switch (format) {
        case BaseMediaDescription::Format::Ogg:
            return create_container_writer<IO, Mixer, BaseMediaDescription::Format::Ogg>(desc, sink);
        case BaseMediaDescription::Format::Matroska:
            return create_container_writer<IO, Mixer, BaseMediaDescription::Format::Matroska>(desc, sink);
        case BaseMediaDescription::Format::Webm:
            return create_container_writer<IO, Mixer, BaseMediaDescription::Format::Webm>(desc, sink);
        case BaseMediaDescription::Format::MP4:
            return create_container_writer<IO, Mixer, BaseMediaDescription::Format::MP4>(desc, sink);
        case BaseMediaDescription::Format::WAV:
            return create_container_writer<IO, Mixer, BaseMediaDescription::Format::WAV>(desc, sink);
        case BaseMediaDescription::Format::FLAC:
            return create_container_writer<IO, Mixer, BaseMediaDescription::Format::FLAC>(desc, sink);
        default:
            return std::make_unique<IO<io::ContainerWriter<Mixer, containers::RawMuxer>>>(desc.input, sink);
        }
    }

    template<template<typename> typename IO, typename Mixer, BaseMediaDescription::Format Format>
    std::unique_ptr<io::BaseWriter> MediaSourceFactory::create_container_writer(const BaseMediaDescription& desc, BaseSink* sink) {
        if constexpr (containers::LibAvMuxer<Format>::supports(Mixer::kKind)) {
            return std::make_unique<IO<io::ContainerWriter<Mixer, containers::LibAvMuxer<Format>>>>(desc.input, sink);
        } else {
            RTC_LOG(LS_ERROR) << "The container can't hold this kind of media";
            throw InvalidParams("The container can't hold this kind of media");
        }
    }

    BaseMediaDescription::Format MediaSourceFactory::input_format(const BaseMediaDescription& desc) {
        const auto format = desc.effective_format();
        if (dynamic_cast<const VideoDescription*>(&desc)) {
            if (format != BaseMediaDescription::Format::Auto && format != BaseMediaDescription::Format::I420 && format != BaseMediaDescription::Format::Matroska && format != BaseMediaDescription::Format::Webm && format != BaseMediaDescription::Format::MP4) {
                RTC_LOG(LS_ERROR) << "Invalid video format";
                throw InvalidParams("Video supports only the Auto, I420, Matroska, Webm and MP4 formats");
            }
        } else if (format == BaseMediaDescription::Format::I420) {
            RTC_LOG(LS_ERROR) << "Invalid audio format";
            throw InvalidParams("Audio doesn't support the I420 format");
        }
        return format;
    }

    BaseMediaDescription::Format MediaSourceFactory::output_format(const BaseMediaDescription& desc) {
        const bool video = dynamic_cast<const VideoDescription*>(&desc) != nullptr;
        auto format = input_format(desc);
        if (format == BaseMediaDescription::Format::Auto) {
            const auto guessed = desc.media_source == BaseMediaDescription::MediaSource::File ? containers::LibAvFormat::from_path(desc.input) : std::nullopt;
            format = guessed.value_or(video ? BaseMediaDescription::Format::Matroska : BaseMediaDescription::Format::Ogg);
        }
        if (video && format != BaseMediaDescription::Format::I420 && format != BaseMediaDescription::Format::Matroska && format != BaseMediaDescription::Format::Webm && format != BaseMediaDescription::Format::MP4) {
            RTC_LOG(LS_ERROR) << "Invalid video output format";
            throw InvalidParams("Video can be written only as I420, Matroska, Webm or MP4");
        }
        return format;
    }
} // ntgcalls::media
