//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <concepts>
#include <string>
#include <ntgcalls/io/base_stream.hpp>
#include <ntgcalls/io/file_stream.hpp>
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

        size_t read_some(uint8_t* data, const size_t size) override {
            return Stream::read_some(data, size);
        }

        std::optional<int64_t> seek(const int64_t offset, const int whence) override {
            return Stream::seek(offset, whence);
        }

    public:
        StreamIO(const std::string& input, media::BaseSink* sink): BaseIO(sink), Format(sink), Stream(input, BaseStream::Mode::Read) {
            if constexpr (std::same_as<Stream, FileStream>) {
                Format::prepare();
            }
        }

        ~StreamIO() override {
            Stream::interrupt();
            Format::close();
            Stream::close();
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
