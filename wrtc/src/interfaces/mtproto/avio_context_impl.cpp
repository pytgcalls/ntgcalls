//
// Created by Lauren on 14/04/25.
//

#include <wrtc/interfaces/mtproto/avio_context_impl.hpp>

namespace wrtc::interfaces::mtproto {
    AVIOContextImpl::AVIOContextImpl(bytes::binary&& file_data): file_data_(std::move(file_data)) {
        buffer_.resize(4 * 1024);
        context_ = avio_alloc_context(
            buffer_.data(),
            static_cast<int>(buffer_.size()),
            0,
            this,
            &read,
            nullptr,
            &seek
        );
    }

    AVIOContextImpl::~AVIOContextImpl() {
        avio_context_free(&context_);
    }

    AVIOContext* AVIOContextImpl::get_context() const {
        return context_;
    }

    int AVIOContextImpl::read(void* opaque, unsigned char* buffer, const int buffer_size) {
        const auto instance = static_cast<AVIOContextImpl*>(opaque);

        int bytes_to_read = std::min(buffer_size, static_cast<int>(instance->file_data_.size()) - instance->file_read_position_);
        if (bytes_to_read < 0) {
            bytes_to_read = 0;
        }

        if (bytes_to_read > 0) {
            std::memcpy(buffer, instance->file_data_.data() + instance->file_read_position_, bytes_to_read);
            instance->file_read_position_ += bytes_to_read;
            return bytes_to_read;
        }
        return AVERROR_EOF;
    }

    int64_t AVIOContextImpl::seek(void* opaque, const int64_t offset, const int whence) {
        const auto instance = static_cast<AVIOContextImpl*>(opaque);

        if (whence == 0x10000) {
            return static_cast<int64_t>(instance->file_data_.size());
        }

        int64_t seek_offset = std::min(offset, static_cast<int64_t>(instance->file_data_.size()));
        if (seek_offset < 0) {
            seek_offset = 0;
        }
        instance->file_read_position_ = static_cast<int>(seek_offset);
        return seek_offset;
    }
} // wrtc::interfaces::mtproto
