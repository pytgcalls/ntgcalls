//
// Created by Laky64 on 07/03/2024.
//

#pragma once
#include <memory>
#include <span>
#include <vector>

namespace bytes {
    using shared_binary = std::shared_ptr<uint8_t[]>;
    using binary = std::vector<uint8_t>;

    using byte = std::byte;
    using vector = std::vector<byte>;
    using span = std::span<byte>;
    using const_span = std::span<const byte>;

    template <size_t Size>
    using array = std::array<byte, Size>;

    template <typename Container,
    typename = std::enable_if_t<!std::is_const_v<Container> && !std::is_same_v<Container, binary>>>
    span make_span(Container &container) {
        return std::as_writable_bytes(span(container.data(), container.size()));
    }

    template <typename Container>
    const_span make_span(const Container &container) {
        return std::as_bytes(make_span(container));
    }

    inline shared_binary make_shared_binary(const size_t size) {
        return std::make_shared<uint8_t[]>(size);
    }

    template <typename Container>
    vector make_vector(const Container &container) {
        const auto buffer = make_span(container);
        return { buffer.begin(), buffer.end() };
    }

    struct memory_span {
        const void* data = nullptr;
        size_t size = 0;

        memory_span(const void *data, const size_t size) :data(data), size(size) {}
    };

    void set_with_const(span destination, byte value);

    void copy(span destination, const_span source);
} // wrtc
