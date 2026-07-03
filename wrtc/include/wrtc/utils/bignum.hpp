//
// Created by Lauren on 07/03/24.
//

#pragma once

#include <openssl/bn.h>
#include <wrtc/utils/binary.hpp>

namespace openssl {
    class Context {
    public:
        Context() : data_(BN_CTX_new()) {}

        Context(const Context &other) = delete;

        ~Context() {
            if (data_) {
                BN_CTX_free(data_);
            }
        }

        [[nodiscard]] BN_CTX* raw() const {
            return data_;
        }
    private:
        BN_CTX* data_ = nullptr;
    };

    class BigNum {
        mutable BIGNUM* data_ = nullptr;
        mutable bool failed_ = false;

        void clear() const;

    public:
        BigNum() = default;

        explicit BigNum(bytes::const_span bytes);

        explicit BigNum(uint32_t word);

        ~BigNum();

        BigNum &operator=(const BigNum &other);

        bool is_negative() const;

        uint32_t bits_size() const;

        uint32_t bytes_size() const;

        void set_mod_exp(const BigNum &base, const BigNum &power, const BigNum &m, const Context &context = Context()) const;

        bool failed() const;

        bytes::binary get_bytes() const;

        BIGNUM* raw() const;

        bool is_zero() const;

        void set_bytes(bytes::const_span bytes) const;

        void set_word(uint32_t word) const;

        void set_sub(const BigNum &a, const BigNum &b) const;
    };
} // openssl
