//
// Created by Lauren on 07/03/24.
//

#pragma once
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace bytes {
    using byte = uint8_t;
    using unique_binary = std::unique_ptr<byte[]>;
    using binary = std::vector<byte>;

    using span = std::span<byte>;
    using const_span = std::span<const byte>;

    template <size_t Size>
    using array = std::array<byte, Size>;

    template <typename Container,
    typename = std::enable_if_t<!std::is_const_v<Container> && !std::is_same_v<Container, binary>>>
    span make_span(Container &container) {
        const auto raw = std::as_writable_bytes(std::span(container.data(), container.size()));
        return { reinterpret_cast<byte*>(raw.data()), raw.size() };
    }

    template <typename Container>
    const_span make_span(const Container &container) {
        const auto raw = std::as_bytes(std::span(container.data(), container.size()));
        return { reinterpret_cast<const byte*>(raw.data()), raw.size() };
    }

    inline const_span view(const void* data, const size_t size) {
        // ReSharper disable once CppDFALocalValueEscapesFunction
        return { static_cast<const byte*>(data), size };
    }

    template <typename Container>
    const_span view(const Container &container) {
        const auto raw = std::as_bytes(std::span(container.data(), container.size()));
        return { reinterpret_cast<const byte*>(raw.data()), raw.size() };
    }

    inline unique_binary make_unique_binary(const size_t size) {
        return std::make_unique<uint8_t[]>(size);
    }

    template <typename Container>
    binary make_binary(const Container &container) {
        return { container.begin(), container.end() };
    }

    inline std::optional<binary> make_binary_optional(const void* data, const size_t size) {
        if (data == nullptr) {
            return std::nullopt;
        }
        return binary(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size);
    }

    std::string to_string(const binary &buffer);

    struct memory_span { // NOLINT(*-identifier-naming)
        const void* data = nullptr;
        size_t size = 0;

        memory_span(const void* data, const size_t size) :data(data), size(size) {}
    };

    void set_with_const(span destination, byte value);

    void copy(span destination, const_span source);
} // wrtc
