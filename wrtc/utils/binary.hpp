//
// Created by Laky64 on 07/03/2024.
//

#pragma once
#include <memory>
#include <stdexcept>
#include <openssl/sha.h>

namespace bytes {
    class binary: public std::shared_ptr<uint8_t[]> {
        size_t _s;
    public:

        binary() : std::shared_ptr<uint8_t[]>(nullptr), _s(0) {}

        binary(uint8_t* data, const size_t size): std::shared_ptr<uint8_t[]>(data), _s(size) {}

        explicit binary(const size_t size): std::shared_ptr<uint8_t[]>(new uint8_t[size]), _s(size) {}

        // ReSharper disable once CppNonExplicitConvertingConstructor
        binary(std::nullptr_t): std::shared_ptr<uint8_t[]>(nullptr), _s(0) {} // NOLINT(*-explicit-constructor)

        [[nodiscard]] bool empty() const;

        [[nodiscard]] size_t size() const;

        [[nodiscard]] binary subspan(size_t start, size_t count) const;

        [[nodiscard]] binary Sha256() const;

        [[nodiscard]] binary Sha1() const;
    };

    void set_with_const(const binary& destination, uint8_t value);

    void copy(const binary& destination, const binary& source);
} // wrtc
