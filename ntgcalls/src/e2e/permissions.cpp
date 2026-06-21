//
// Created by Laky-64 on 17/06/26.
//

#include <ntgcalls/e2e/permissions.hpp>

namespace telegram::e2e {
    bool Permissions::mayAddUsers() const {
        return (flags & AddUsers) != 0;
    }

    bool Permissions::mayRemoveUsers() const {
        return (flags & RemoveUsers) != 0;
    }

    bool Permissions::maySetValue() const {
        return (flags & SetValue) != 0;
    }

    bool Permissions::isParticipant() const {
        return (flags & IsParticipant) != 0;
    }

    bool Permissions::mayChangeSharedKey() const {
        return isParticipant() && (mayRemoveUsers() || mayAddUsers());
    }
} // telegram::e2e