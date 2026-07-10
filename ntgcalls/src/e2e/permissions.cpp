//
// Created by Lauren on 17/06/26.
//

#include <ntgcalls/e2e/permissions.hpp>

namespace ntgcalls::e2e {
    bool Permissions::may_add_users() const {
        return (flags & AddUsers) != 0;
    }

    bool Permissions::may_remove_users() const {
        return (flags & RemoveUsers) != 0;
    }

    bool Permissions::may_set_value() const {
        return (flags & SetValue) != 0;
    }

    bool Permissions::is_participant() const {
        return (flags & IsParticipant) != 0;
    }

    bool Permissions::may_change_shared_key() const {
        return is_participant() && (may_remove_users() || may_add_users());
    }
} // ntgcalls::e2e