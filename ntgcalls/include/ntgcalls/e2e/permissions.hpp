//
// Created by Laky-64 on 17/06/26.
//

#pragma once
#include <cstdint>

namespace telegram::e2e {
    struct Permissions {
        enum GroupParticipantFlags : int32_t {
            AddUsers = 1 << 0,
            RemoveUsers = 1 << 1,
            SetValue = 1 << 2,
            AllPermissions = (1 << 3) - 1,
            IsParticipant = 1 << 30
        };

        int32_t flags = 0;

        [[nodiscard]] bool mayAddUsers() const;

        [[nodiscard]] bool mayRemoveUsers() const;

        [[nodiscard]] bool maySetValue() const;

        [[nodiscard]] bool isParticipant() const;

        [[nodiscard]] bool mayChangeSharedKey() const;
    };
} // telegram::e2e
