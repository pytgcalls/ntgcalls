//
// Created by Laky64 on 07/03/2024.
//

#pragma once

#include <openssl/bn.h>
#include <openssl/sha.h>
#include "binary.hpp"

namespace openssl {
    class Context {
    public:
        Context() : _data(BN_CTX_new()) {}

        Context(const Context &other) = delete;

        ~Context() {
            if (_data) {
                BN_CTX_free(_data);
            }
        }

        [[nodiscard]] BN_CTX *raw() const {
            return _data;
        }
    private:
        BN_CTX *_data = nullptr;
    };

    class BigNum {
        mutable BIGNUM *_data = nullptr;
        mutable bool _failed = false;

        void clear() const;
    public:
        BigNum() = default;

        explicit BigNum(const bytes::binary& bytes);

        explicit BigNum(uint32_t word);

        ~BigNum();

        BigNum &operator=(const BigNum &other);

        bool isNegative() const;

        uint32_t bitsSize() const;

        uint32_t bytesSize() const;

        void setModExp(const BigNum &base, const BigNum &power, const BigNum &m, const Context &context = Context()) const;

        bool failed() const;

        bytes::binary getBytes() const;

        BIGNUM *raw() const;

        bool isZero() const;

        void setBytes(const bytes::binary& bytes) const;

        void setWord(uint32_t word) const;

        void setSub(const BigNum &a, const BigNum &b) const;
    };

    inline bytes::binary Sha256(const bytes::binary& data) {
        auto bytes = bytes::binary(SHA256_DIGEST_LENGTH);
        SHA256(data.get(), SHA256_DIGEST_LENGTH, bytes.get());
        return bytes;
    }

} // openssl
