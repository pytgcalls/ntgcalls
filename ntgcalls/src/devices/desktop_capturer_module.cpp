//
// Created by Laky64 on 15/10/24.
//

#if !defined(IS_ANDROID) && !defined(IS_MACOS)
#include <thread>
#include <ntgcalls/exceptions.hpp>
#include <third_party/libyuv/include/libyuv.h>
#include <ntgcalls/utils/g_lib_loop_manager.hpp>
#include <ntgcalls/devices/desktop_capturer_module.hpp>
#include <modules/desktop_capture/desktop_capturer_differ_wrapper.h>

namespace ntgcalls {
    DesktopCapturerModule::DesktopCapturerModule(const VideoDescription& desc, BaseSink* sink): BaseIO(sink), BaseReader(sink), desc(desc) {
        capturer = CreateCapturer();
        try {
            auto sourceMetadata = json::parse(desc.input);
            capturer->SelectSource(sourceMetadata["id"].get<webrtc::DesktopCapturer::SourceId>());
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        capturer->SetMaxFrameRate(desc.fps);
    }

    DesktopCapturerModule::~DesktopCapturerModule() {
        running = false;
        thread.Finalize();
        GLibLoopManager::RemoveInstance();
    }

    std::unique_ptr<webrtc::DesktopCapturer> DesktopCapturerModule::CreateCapturer() {
        auto options = webrtc::DesktopCaptureOptions::CreateDefault();
        options.set_detect_updated_region(true);
#ifdef IS_WINDOWS
        options.set_allow_directx_capturer(true);
#elif IS_MACOS
        options.set_allow_iosurface(true);
#elif IS_LINUX
        options.set_allow_pipewire(true);
#endif
        return webrtc::DesktopCapturer::CreateGenericCapturer(options);
    }

    void DesktopCapturerModule::OnCaptureResult(const webrtc::DesktopCapturer::Result result, const std::unique_ptr<webrtc::DesktopFrame> frame) {
        if (!enabled) return;
        if (result == webrtc::DesktopCapturer::Result::SUCCESS) {
            const int width = frame->size().width();
            const int height = frame->size().height();

            const auto ySize = width * height;
            const auto uvSize = ySize / 4;
            const auto yPlane = std::make_unique<uint8_t[]>(ySize);
            const auto uPlane = std::make_unique<uint8_t[]>(uvSize);
            const auto vPlane = std::make_unique<uint8_t[]>(uvSize);
            libyuv::ARGBToI420(
                frame->data(), frame->stride(),
                yPlane.get(), width,
                uPlane.get(), width / 2,
                vPlane.get(), width / 2,
                width, height
            );

            const auto yScaledSize = desc.width * desc.height;
            const auto uvScaledSize = yScaledSize / 4;
            auto yuv = bytes::make_unique_binary(yScaledSize + uvScaledSize * 2);

            if (desc.width == width && desc.height == height) {
                memcpy(yuv.get(), yPlane.get(), ySize);
                memcpy(yuv.get() + ySize, uPlane.get(), uvSize);
                memcpy(yuv.get() + ySize + uvSize, vPlane.get(), uvSize);
            } else {
                const auto yScaledPlane = std::make_unique<uint8_t[]>(yScaledSize);
                const auto uScaledPlane = std::make_unique<uint8_t[]>(uvScaledSize);
                const auto vScaledPlane = std::make_unique<uint8_t[]>(uvScaledSize);

                I420Scale(
                    yPlane.get(), width,
                    uPlane.get(), width / 2,
                    vPlane.get(), width / 2,
                    width, height,
                    yScaledPlane.get(), desc.width,
                    uScaledPlane.get(), desc.width / 2,
                    vScaledPlane.get(), desc.width / 2,
                    desc.width, desc.height,
                    libyuv::kFilterBox
                );

                memcpy(yuv.get(), yScaledPlane.get(), yScaledSize);
                memcpy(yuv.get() + yScaledSize, uScaledPlane.get(), uvScaledSize);
                memcpy(yuv.get() + yScaledSize + uvScaledSize, vScaledPlane.get(), uvScaledSize);
            }

            (void) dataCallback(std::move(yuv), {
                0,
                webrtc::kVideoRotation_0,
                static_cast<uint16_t>(desc.width),
                static_cast<uint16_t>(desc.height),
            });
        } else if (result == webrtc::DesktopCapturer::Result::ERROR_PERMANENT) {
            (void) eofCallback();
        }
    }


    std::vector<DeviceInfo> DesktopCapturerModule::GetSources() {
        const auto capturer = CreateCapturer();
        if (!capturer) {
            throw MediaDeviceError("Failed to create desktop capturer");
        }
        webrtc::DesktopCapturer::SourceList sources;
        capturer->GetSourceList(&sources);
        std::vector<DeviceInfo> devices;
        for (const auto& [id, title, display_id] : sources) {
            const json metadata = {
                {"id", id},
                {"display_id", display_id}
            };
            devices.emplace_back(title.empty() ? "Screen" : title, metadata.dump());
        }
        return devices;
    }

    void DesktopCapturerModule::open() {
        if (running) return;
        running = true;
        GLibLoopManager::AddInstance();
        capturer->Start(this);
        capturer->CaptureFrame();
        thread = rtc::PlatformThread::SpawnJoinable(
            [this] {
                while (running) {
                    std::this_thread::sleep_for(sink->frameTime());
                    capturer->CaptureFrame();
                }
            },
            "DesktopCapturerModule",
            rtc::ThreadAttributes().SetPriority(rtc::ThreadPriority::kRealtime)
        );
    }

    bool DesktopCapturerModule::IsSupported() {
        return CreateCapturer() != nullptr;
    }
} // ntgcalls

#endif