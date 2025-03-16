//
// Created by Laky64 on 07/03/2024.
//

#include <wrtc/utils/bignum.hpp>

namespace openssl {
    void BigNum::clear() const {
        BN_clear_free(std::exchange(_data, nullptr));
    }

    BIGNUM* BigNum::raw() const {
        if (!_data) _data = BN_new();
        return _data;
    }

    bool BigNum::isZero() const {
        return !failed() && (!_data || BN_is_zero(raw()));
    }

    bool BigNum::failed() const {
        return _failed;
    }

    void BigNum::setBytes(const bytes::const_span bytes) const {
        if (bytes.empty()) {
            clear();
            _failed = false;
        } else {
            _failed = !BN_bin2bn(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), raw());
        }
    }

    void BigNum::setWord(const uint32_t word) const {
        if (!word) {
            clear();
            _failed = false;
        } else {
            _failed = !BN_set_word(raw(), word);
        }
    }

    void BigNum::setModExp(const BigNum& base, const BigNum& power, const BigNum& m, const Context &context) const {
        if (base.failed() || power.failed() || m.failed()) {
            _failed = true;
        } else if (base.isNegative() || power.isNegative() || m.isNegative()) {
            _failed = true;
        } else if (!BN_mod_exp(raw(), base.raw(), power.raw(), m.raw(), context.raw())) {
            _failed = true;
        } else if (isNegative()) {
            _failed = true;
        } else {
            _failed = false;
        }
    }

    void BigNum::setSub(const BigNum& a, const BigNum& b) const {
        if (a.failed() || b.failed()) {
            _failed = true;
        } else {
            _failed = !BN_sub(raw(), a.raw(), b.raw());
        }
    }

    BigNum::BigNum(const bytes::const_span bytes): BigNum() {
        setBytes(bytes);
    }

    BigNum::BigNum(const uint32_t word): BigNum() {
        setWord(word);
    }

    BigNum::~BigNum() {
        clear();
    }

    BigNum& BigNum::operator=(const BigNum& other) { // NOLINT(*-unhandled-self-assignment)
        if (other.failed()) {
            _failed = true;
        } else if (other.isZero()) {
            clear();
            _failed = false;
        } else if (!_data) {
            _data = BN_dup(other.raw());
            _failed = false;
        } else {
            _failed = !BN_copy(raw(), other.raw());
        }
        return *this;
    }

    bool BigNum::isNegative() const {
        return !failed() && _data && BN_is_negative(raw());
    }

    uint32_t BigNum::bitsSize() const {
        return failed() ? 0 : BN_num_bits(raw());
    }

    uint32_t BigNum::bytesSize() const {
        return failed() ? 0 : BN_num_bytes(raw());
    }

    bytes::vector BigNum::getBytes() const {
        if (failed()) {
            return {};
        }
        const auto length = BN_num_bytes(raw());
        bytes::vector result(length);
        BN_bn2bin(raw(), reinterpret_cast<unsigned char*>(result.data()));
        return result;
    }
} // openssl