//
// Created by Lauren on 21/06/26.
//

#pragma once
#include <functional>
#include <string>

namespace ntgcalls::instances {
    class E2EInterface {
    public:
        virtual ~E2EInterface() = default;

        virtual void on_update_emojis(const std::function<void(std::string)>& callback) = 0;

        virtual std::string get_fingerprint_emojis() = 0;
    };
} // ntgcalls::instances
