//
// Created by Laky64 on 22/04/25.
//

#pragma once
#include <deque>
#include <dlfcn.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace wrtc {

    class HardwareAcceleration {
        class AvErrorWrap {
        public:
            explicit AvErrorWrap(const int code = 0) : _code(code) {}

            explicit operator bool() const;

            std::string message() const;

        private:
            int _code = 0;
        };

        template <auto fn>
        struct custom_delete {
            template <typename T>
            constexpr void operator()(T* value) const {
                if (value) {
                    fn(value);
                }
            }
        };

        using LibraryHandle = std::unique_ptr<void, custom_delete<dlclose>>;

        static bool checkAvFormats(const AVPixelFormat *formats, AVPixelFormat format);

        static AVHWDeviceType getHwDeviceType(AVPixelFormat format);

        static bool initHardware(AVCodecContext *context, AVHWDeviceType type);

        static std::deque<AVPixelFormat> checkHardwareLibs();

        static LibraryHandle tryLoadLibrary(const char *name);

    public:
        static AVPixelFormat GetHwFormat(AVCodecContext *context, const AVPixelFormat *formats);
    };

} // wrtc
