//
// Created by Lauren on 15/03/24.
//

#include <vector>
#include <zlib.h>
#include <wrtc/utils/g_zip.hpp>

namespace bytes {
    bool GZip::is_gzip(const binary& data) {
        if (data.size() < 2) {
            return false;
        }
        return (data[0] == 0x1f && data[1] == 0x8b) || (data[0] == 0x78 && data[1] == 0x9c);
    }

    binary GZip::zip(const binary& data) {
        z_stream stream;
        stream.zalloc = nullptr;
        stream.zfree = nullptr;
        stream.opaque = nullptr;
        stream.avail_in = static_cast<uint32_t>(data.size());
        stream.next_in = const_cast<unsigned char*>(data.data());
        stream.total_out = 0;
        stream.avail_out = 0;
        binary output;
        if (constexpr int kCompression = 9; deflateInit2(&stream, kCompression, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) == Z_OK) {
            output.resize(kChunkSize);

            while (stream.avail_out == 0) {
                if (stream.total_out >= output.size()) {
                    output.resize(output.size() + kChunkSize);
                }
                stream.next_out = output.data() + stream.total_out;
                stream.avail_out = static_cast<uint32_t>(output.size() - stream.total_out);
                deflate(&stream, Z_FINISH);
            }
            deflateEnd(&stream);
            output.resize(stream.total_out);
        }
        return output;
    }

    std::optional<binary> GZip::unzip(const binary& data, const size_t size_limit) {
        z_stream stream;
        stream.zalloc = nullptr;
        stream.zfree = nullptr;
        stream.avail_in = static_cast<uint32_t>(data.size());
        stream.next_in = const_cast<unsigned char*>(data.data());
        stream.total_out = 0;
        stream.avail_out = 0;

        binary output;
        if (inflateInit2(&stream, 47) == Z_OK) {
            int status = Z_OK;
            output.resize(data.size() * 2);
            while (status == Z_OK) {
                if (size_limit > 0 && stream.total_out > size_limit) {
                    return std::nullopt;
                }
                if (stream.total_out >= output.size()) {
                    output.resize(output.size() + data.size() / 2);
                }
                stream.next_out = output.data() + stream.total_out;
                stream.avail_out = static_cast<uint32_t>(output.size() - stream.total_out);
                status = inflate(&stream, Z_SYNC_FLUSH);
            }
            if (inflateEnd(&stream) == Z_OK) {
                if (status == Z_STREAM_END) {
                    output.resize(stream.total_out);
                } else if (size_limit > 0 && output.size() > size_limit) {
                    return std::nullopt;
                }
            }
        }
        return output;
    }
} // bytes