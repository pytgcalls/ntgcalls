//
// Created by Laky64 on 07/10/24.
//

#pragma once

namespace ntgcalls {

    class BaseReceiver {
    public:
        virtual ~BaseReceiver() = default;

        virtual void open() = 0;
    };

} // ntgcalls
