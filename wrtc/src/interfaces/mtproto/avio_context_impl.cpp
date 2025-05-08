//
// Created by Laky64 on 14/04/25.
//

#include <wrtc/interfaces/mtproto/avio_context_impl.hpp>

namespace wrtc {
    AVIOContextImpl::AVIOContextImpl(bytes::binary&& fileData) : fileData(std::move(fileData)) {
        buffer.resize(4 * 1024);
        context = avio_alloc_context(
            buffer.data(),
            static_cast<int>(buffer.size()),
            0,
            this,
            &AVIOContextImplRead,
            nullptr,
            &AVIOContextImplSeek
        );
    }

    AVIOContextImpl::~AVIOContextImpl() {
        avio_context_free(&context);
    }

    AVIOContext* AVIOContextImpl::getContext() const {
        return context;
    }

    int AVIOContextImpl::AVIOContextImplRead(void* opaque, unsigned char* buffer, const int bufferSize) {
        const auto instance = static_cast<AVIOContextImpl *>(opaque);

        int bytesToRead = std::min(bufferSize, static_cast<int>(instance->fileData.size()) - instance->fileReadPosition);
        if (bytesToRead < 0) {
            bytesToRead = 0;
        }

        if (bytesToRead > 0) {
            memcpy(buffer, instance->fileData.data() + instance->fileReadPosition, bytesToRead);
            instance->fileReadPosition += bytesToRead;
            return bytesToRead;
        }
        return AVERROR_EOF;
    }

    int64_t AVIOContextImpl::AVIOContextImplSeek(void* opaque, const int64_t offset, const int whence) {
        const auto instance = static_cast<AVIOContextImpl *>(opaque);

        if (whence == 0x10000) {
            return static_cast<int64_t>(instance->fileData.size());
        }

        int64_t seekOffset = std::min(offset, static_cast<int64_t>(instance->fileData.size()));
        if (seekOffset < 0) {
            seekOffset = 0;
        }
        instance->fileReadPosition = static_cast<int>(seekOffset);
        return seekOffset;
    }
} // wrtc