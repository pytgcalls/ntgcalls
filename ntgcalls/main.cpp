#include <iostream>
#include "ffmpeg/ffmpeg.hpp"

int main() {
    ntgcalls::MediaDescription desc;
    std::cout << "WORK";
    desc.audio = {
            48000,
            16,
            2,
            "C:\\Users\\iraci\\PycharmProjects\\NativeTgCalls\\tools\\test.mp3",
    };
    auto test = ntgcalls::FFmpeg(desc);
    std::cout << "WORK";
    return 0;
}