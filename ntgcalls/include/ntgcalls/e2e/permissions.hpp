//
// Created by Lauren on 17/06/26.
//

#pragma once
#include <cstdint>

namespace ntgcalls::e2e {
    struct Permissions {
        enum GroupParticipantFlags : int32_t {
            AddUsers = 1 << 0,
            RemoveUsers = 1 << 1,
            SetValue = 1 << 2,
            AllPermissions = (1 << 3) - 1,
            IsParticipant = 1 << 30
        };

        int32_t flags = 0;

        [[nodiscard]] bool may_add_users() const;

        [[nodiscard]] bool may_remove_users() const;

        [[nodiscard]] bool may_set_value() const;

        [[nodiscard]] bool is_participant() const;

        [[nodiscard]] bool may_change_shared_key() const;
    };
} // ntgcalls::e2e
