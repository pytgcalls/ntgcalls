//
// Created by Laky64 on 22/08/2023.
//

#include <cstdint>
#include "client.hpp"
#include "models/media_description.hpp"

namespace ntgcalls {

    class NTgCalls {
    private:
        std::map<int64_t, Client> connections;

        bool exists(int64_t chatId);

        Client safeConnection(int64_t chatId);

    public:
        NTgCalls();

        std::string createCall(int64_t chatId, MediaDescription media);

        void connect(int64_t chatId, std::string params);

        void changeStream(int64_t chatId, MediaDescription media);

        void pause(int64_t chatId);

        void resume(int64_t chatId);

        void mute(int64_t chatId);

        void unmute(int64_t chatId);

        void stop(int64_t chatId);
    };

} // ntgcalls

