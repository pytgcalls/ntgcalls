//
// Created by Laky-64 on 21/06/26.
//

#pragma once
#include <functional>
#include <string>

namespace ntgcalls {
    class E2EInterface {
    public:
        virtual ~E2EInterface() = default;

        virtual void onUpdateEmojis(const std::function<void(std::string)>& callback) = 0;

        virtual std::string getFingerprintEmojis() = 0;
    };
} // ntgcalls
