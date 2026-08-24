//
// C binding support helpers (hand-written, target-level).
//

#pragma once

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <wrtc/utils/binary.hpp>

static inline char* ntg_dup_string(const std::string& value) {
    const auto size = value.size();
    const auto out = static_cast<char*>(malloc(size + 1));
    memcpy(out, value.data(), size);
    out[size] = '\0';
    return out;
}

static inline void ntg_dup_bytes(const bytes::binary& value, uint8_t** out, size_t* len) {
    *len = value.size();
    if (value.empty()) {
        *out = nullptr;
        return;
    }
    *out = static_cast<uint8_t*>(malloc(value.size()));
    memcpy(*out, value.data(), value.size());
}

static inline bytes::binary ntg_to_cpp_bytes(const uint8_t* data, const size_t len) {
    return {data, data + len};
}

static inline int8_t ntg_elem_to_c(const int8_t value) {
    return value;
}
static inline uint8_t ntg_elem_to_c(const uint8_t value) {
    return value;
}
static inline int16_t ntg_elem_to_c(const int16_t value) {
    return value;
}
static inline uint16_t ntg_elem_to_c(const uint16_t value) {
    return value;
}
static inline int32_t ntg_elem_to_c(const int32_t value) {
    return value;
}
static inline uint32_t ntg_elem_to_c(const uint32_t value) {
    return value;
}
static inline int64_t ntg_elem_to_c(const int64_t value) {
    return value;
}
static inline uint64_t ntg_elem_to_c(const uint64_t value) {
    return value;
}
static inline double ntg_elem_to_c(const double value) {
    return value;
}
static inline bool ntg_elem_to_c(const bool value) {
    return value;
}
static inline char* ntg_elem_to_c(const std::string& value) {
    return ntg_dup_string(value);
}

static inline int8_t ntg_elem_from_c(const int8_t value) {
    return value;
}
static inline uint8_t ntg_elem_from_c(const uint8_t value) {
    return value;
}
static inline int16_t ntg_elem_from_c(const int16_t value) {
    return value;
}
static inline uint16_t ntg_elem_from_c(const uint16_t value) {
    return value;
}
static inline int32_t ntg_elem_from_c(const int32_t value) {
    return value;
}
static inline uint32_t ntg_elem_from_c(const uint32_t value) {
    return value;
}
static inline int64_t ntg_elem_from_c(const int64_t value) {
    return value;
}
static inline uint64_t ntg_elem_from_c(const uint64_t value) {
    return value;
}
static inline double ntg_elem_from_c(const double value) {
    return value;
}
static inline bool ntg_elem_from_c(const bool value) {
    return value;
}
static inline std::string ntg_elem_from_c(const char* value) {
    return value ? std::string(value) : std::string();
}
