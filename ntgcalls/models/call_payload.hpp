//
// Created by Laky64 on 21/08/2023.
//

#pragma once

#include <wrtc/wrtc.hpp>
#include <nlohmann/json.hpp>


namespace ntgcalls {
    using nlohmann::json;

    class CallPayload {
    public:
        std::string ufrag;
        std::string pwd;
        std::string hash;
        std::string setup;
        std::string fingerprint;
        bool isGroup;
        wrtc::TgSSRC audioSource;
        std::vector<wrtc::TgSSRC> sourceGroups;

        explicit CallPayload(const wrtc::Description &desc, bool isGroup);

        // ReSharper disable once CppNonExplicitConversionOperator
        operator std::string() const; // NOLINT(*-explicit-constructor)

        // ReSharper disable once CppNonExplicitConversionOperator
        operator bytes::binary() const; // NOLINT(*-explicit-constructor)
    };

} // ntgcalls

