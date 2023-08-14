//
// Created by Laky64 on 13/08/2023.
//

#include <iostream>
#include "client.hpp"

int main() {
    auto client = ntgcalls::Client();
    std::cout << client.createCall("C:/Users/iraci/PycharmProjects/NativeTgCalls/tools/output.pcm");
    return 0;
}