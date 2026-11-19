//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <concepts>
#include <string>
#include <ntgcalls/io/base_stream.hpp>
#include <ntgcalls/io/threaded_reader.hpp>
#include <ntgcalls/io/threaded_writer.hpp>

namespace ntgcalls::io {
    template<typename Format, std::derived_from<BaseStream> Stream>
    class StreamIO;

    template<std::derived_from<ThreadedReader> Format, std::derived_from<BaseStream> Stream>
    class StreamIO<Format, Stream> final: public Format, Stream {
    protected:
        bytes::unique_binary read(const int64_t size) override {
            return Stream::read(size);
        }

    public:
        StreamIO(const std::string& input, media::BaseSink* sink): BaseIO(sink), Format(sink), Stream(input, BaseStream::Mode::Read) {}

        ~StreamIO() override {
            Stream::close();
            Format::close();
        }
    };

    template<std::derived_from<ThreadedWriter> Format, std::derived_from<BaseStream> Stream>
    class StreamIO<Format, Stream> final: public Format, Stream {
    protected:
        void write(const bytes::unique_binary& data, const size_t size) override {
            Stream::write(data, size);
        }

    public:
        StreamIO(const std::string& output, media::BaseSink* sink): BaseIO(sink), Format(sink), Stream(output, BaseStream::Mode::Write) {}

        ~StreamIO() override {
            Format::flush();
            Format::close();
            Stream::close();
        }
    };
} // ntgcalls::io
