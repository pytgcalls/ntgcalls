//
// Created by Lauren on 07/03/24.
//

#include <utility>
#include <wrtc/utils/bignum.hpp>

namespace openssl {
    void BigNum::clear() const {
        BN_clear_free(std::exchange(data_, nullptr));
    }

    BIGNUM* BigNum::raw() const {
        if (!data_) data_ = BN_new();
        return data_;
    }

    bool BigNum::is_zero() const {
        return !failed() && (!data_ || BN_is_zero(raw()));
    }

    bool BigNum::failed() const {
        return failed_;
    }

    void BigNum::set_bytes(const bytes::const_span bytes) const {
        if (bytes.empty()) {
            clear();
            failed_ = false;
        } else {
            failed_ = !BN_bin2bn(bytes.data(), bytes.size(), raw());
        }
    }

    void BigNum::set_word(const uint32_t word) const {
        if (!word) {
            clear();
            failed_ = false;
        } else {
            failed_ = !BN_set_word(raw(), word);
        }
    }

    void BigNum::set_mod_exp(const BigNum& base, const BigNum& power, const BigNum& m, const Context& context) const {
        if (base.failed() || power.failed() || m.failed()) {
            failed_ = true;
        } else if (base.is_negative() || power.is_negative() || m.is_negative()) {
            failed_ = true;
        } else if (!BN_mod_exp(raw(), base.raw(), power.raw(), m.raw(), context.raw())) {
            failed_ = true;
        } else if (is_negative()) {
            failed_ = true;
        } else {
            failed_ = false;
        }
    }

    void BigNum::set_sub(const BigNum& a, const BigNum& b) const {
        if (a.failed() || b.failed()) {
            failed_ = true;
        } else {
            failed_ = !BN_sub(raw(), a.raw(), b.raw());
        }
    }

    BigNum::BigNum(const bytes::const_span bytes): BigNum() {
        set_bytes(bytes);
    }

    BigNum::BigNum(const uint32_t word): BigNum() {
        set_word(word);
    }

    BigNum::~BigNum() {
        clear();
    }

    BigNum& BigNum::operator=(const BigNum& other) { // NOLINT(*-unhandled-self-assignment)
        if (other.failed()) {
            failed_ = true;
        } else if (other.is_zero()) {
            clear();
            failed_ = false;
        } else if (!data_) {
            data_ = BN_dup(other.raw());
            failed_ = false;
        } else {
            failed_ = !BN_copy(raw(), other.raw());
        }
        return *this;
    }

    bool BigNum::is_negative() const {
        return !failed() && data_ && BN_is_negative(raw());
    }

    uint32_t BigNum::bits_size() const {
        return failed() ? 0 : BN_num_bits(raw());
    }

    uint32_t BigNum::bytes_size() const {
        return failed() ? 0 : BN_num_bytes(raw());
    }

    bytes::binary BigNum::get_bytes() const {
        if (failed()) {
            return {};
        }
        const auto length = BN_num_bytes(raw());
        bytes::binary result(length);
        BN_bn2bin(raw(), result.data());
        return result;
    }
} // openssl
