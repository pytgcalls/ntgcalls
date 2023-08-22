//
// Created by Laky64 on 22/08/2023.
//

#include <cstdint>
#include "client.hpp"

namespace ntgcalls {

    class NTgCalls {
    private:
        std::map<int64_t, Client> connections;

    public:
        void init(int64_t chatId);
    };

} // ntgcalls

