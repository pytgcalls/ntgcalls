//
// Created by Lauren on 27/11/26.
//

#include <ntgcalls/media/containers/bitstream.hpp>

namespace ntgcalls::media::containers {
    std::optional<std::pair<size_t, size_t>> Bitstream::next_nal(const uint8_t* data, const size_t size, size_t offset) {
        while (offset + 3 <= size && !(data[offset] == 0 && data[offset + 1] == 0 && data[offset + 2] == 1)) {
            ++offset;
        }
        if (offset + 3 >= size) {
            return std::nullopt;
        }
        const auto start = offset + 3;
        auto end = start;
        while (end + 3 <= size && !(data[end] == 0 && data[end + 1] == 0 && data[end + 2] == 1)) {
            ++end;
        }
        if (end + 3 > size) {
            end = size;
        }
        while (end > start && data[end - 1] == 0) {
            --end;
        }
        return std::pair{start, end};
    }

    std::optional<std::pair<uint8_t, size_t>> Bitstream::next_obu(const uint8_t* data, const size_t size, size_t offset) {
        if (offset >= size) {
            return std::nullopt;
        }
        const auto header = data[offset];
        offset += header & 0x04 ? 2 : 1;
        if (!(header & 0x02)) {
            return std::nullopt;
        }
        uint64_t length = 0;
        for (int i = 0; i < 8 && offset < size; i++) {
            const auto byte = data[offset++];
            length |= static_cast<uint64_t>(byte & 0x7F) << (i * 7);
            if (!(byte & 0x80)) {
                break;
            }
        }
        return std::pair{static_cast<uint8_t>(header >> 3 & 0x0F), static_cast<size_t>(offset + length)};
    }

    bool Bitstream::has_nal_type(const uint8_t* data, const size_t size, const uint8_t type) {
        auto nal = next_nal(data, size, 0);
        while (nal) {
            if ((data[nal->first] & 0x1F) == type) {
                return true;
            }
            nal = next_nal(data, size, nal->second);
        }
        return false;
    }

    bool Bitstream::has_obu_type(const uint8_t* data, const size_t size, const uint8_t type) {
        size_t offset = 0;
        while (offset < size) {
            if ((data[offset] >> 3 & 0x0F) == type) {
                return true;
            }
            const auto obu = next_obu(data, size, offset);
            if (!obu) {
                return false;
            }
            offset = obu->second;
        }
        return false;
    }

    bytes::binary Bitstream::h264_parameter_sets(const uint8_t* data, const size_t size) {
        bytes::binary result;
        auto nal = next_nal(data, size, 0);
        while (nal) {
            if (const auto type = data[nal->first] & 0x1F; type == 7 || type == 8) {
                result.insert(result.end(), {0, 0, 0, 1});
                result.insert(result.end(), data + nal->first, data + nal->second);
            }
            nal = next_nal(data, size, nal->second);
        }
        return result;
    }

    bytes::binary Bitstream::av1_sequence_header(const uint8_t* data, const size_t size) {
        size_t offset = 0;
        while (const auto obu = next_obu(data, size, offset)) {
            if (obu->first == 1 && obu->second <= size) {
                return {data + offset, data + obu->second};
            }
            offset = obu->second;
        }
        return {};
    }
} // ntgcalls::media::containers
