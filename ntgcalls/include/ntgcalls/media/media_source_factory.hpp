//
// Created by Lauren on 07/10/24.
//

#pragma once
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/io/base_writer.hpp>
#include <ntgcalls/media/media_description.hpp>

namespace ntgcalls::media {

    class MediaSourceFactory {
        static BaseMediaDescription::Format input_format(const BaseMediaDescription& desc);

        static BaseMediaDescription::Format output_format(const BaseMediaDescription& desc);

        template<template<typename> typename IO>
        static std::unique_ptr<io::BaseReader> create_reader(const BaseMediaDescription& desc, BaseSink* sink);

        template<template<typename> typename IO>
        static std::unique_ptr<io::BaseWriter> create_writer(const BaseMediaDescription& desc, BaseSink* sink);

        template<template<typename> typename IO, typename Mixer>
        static std::unique_ptr<io::BaseWriter> create_mixer_writer(BaseMediaDescription::Format format, const BaseMediaDescription& desc, BaseSink* sink);

        template<template<typename> typename IO, typename Mixer, BaseMediaDescription::Format Format>
        static std::unique_ptr<io::BaseWriter> create_container_writer(const BaseMediaDescription& desc, BaseSink* sink);

    public:
        static std::unique_ptr<io::BaseReader> from_input(const BaseMediaDescription& desc, BaseSink* sink);

        static std::unique_ptr<io::BaseWriter> from_output(const BaseMediaDescription& desc, BaseSink* sink);
    };

} // ntgcalls::media
