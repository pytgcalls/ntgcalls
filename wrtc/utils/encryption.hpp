//
// Created by iraci on 09/03/2024.
//
#pragma once

#include "binary.hpp"

namespace openssl {

    class Sha256 {
    public:
        static bytes::binary Digest(const bytes::binary& data);

        static bytes::binary Concat(const bytes::binary& first, const bytes::binary& second);
    };

    class Sha1 {
    public:
        static bytes::binary Digest(const bytes::binary& data);
    };


    class Aes {
    public:
        struct KeyIv {
            bytes::binary key;
            bytes::binary iv;
        };

        static KeyIv PrepareKeyIv(const bytes::binary& key, const bytes::binary& msgKey, int x);

        static void ProcessCtr(const bytes::binary& from, const bytes::binary& to, const KeyIv& keyIv);
    };

} // openssl
