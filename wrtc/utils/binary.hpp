//
// Created by Laky64 on 07/03/2024.
//

#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <algorithm>

namespace bytes {
    class span {
        const void* _data;
        size_t _size;
    public:
        span(const void *data, const size_t size): _data(data), _size(size) {}

        // ReSharper disable once CppNonExplicitConversionOperator
        operator const void*() const; // NOLINT(*-explicit-constructor)

        explicit operator const uint8_t*() const;

        [[nodiscard]] size_t size() const;
    };

    class binary: public std::shared_ptr<uint8_t[]> {
        size_t _s;
    public:
        binary() : std::shared_ptr<uint8_t[]>(nullptr), _s(0) {}

        binary(uint8_t* data, const size_t size): std::shared_ptr<uint8_t[]>(data), _s(size) {}

        binary(const uint8_t* data, size_t size);

        binary(const char* data, size_t size);

        explicit binary(const size_t size): std::shared_ptr<uint8_t[]>(new uint8_t[size]), _s(size) {}

        // ReSharper disable once CppNonExplicitConvertingConstructor
        binary(std::nullptr_t): binary() {} // NOLINT(*-explicit-constructor)

        explicit binary(const std::string& str);

        [[nodiscard]] bool empty() const;

        [[nodiscard]] size_t size() const;

        [[nodiscard]] binary subBytes(size_t start, size_t count) const;

        [[nodiscard]] binary Sha256() const;

        [[nodiscard]] binary Sha1() const;

        [[nodiscard]] binary copy() const;

        // ReSharper disable once CppNonExplicitConversionOperator
        operator void*() const; // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConversionOperator
        operator uint8_t*() const; // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConversionOperator
        operator span() const; // NOLINT(*-explicit-constructor)

        explicit operator char*() const;

        binary operator+(size_t offset) const;

        bool operator!=(const binary& other) const;

        bool operator==(const binary& other) const;
    };

    void set_with_const(const binary& destination, uint8_t value);

    void copy(const binary& destination, const binary& source);
} // wrtc
