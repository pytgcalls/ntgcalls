//
// Created by Laky64 on 07/03/2024.
//

#pragma once
#include <memory>

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
    };

} // wrtc
