//
// Created by Laky64 on 15/10/24.
//

#ifndef IS_ANDROID
#include <ntgcalls/exceptions.hpp>
#include <third_party/libyuv/include/libyuv.h>
#include <ntgcalls/media/video_sink.hpp>
#include <ntgcalls/utils/g_lib_loop_manager.hpp>
#include <ntgcalls/devices/desktop_capturer_module.hpp>
#include <modules/desktop_capture/desktop_capturer_differ_wrapper.h>

namespace ntgcalls {
    DesktopCapturerModule::DesktopCapturerModule(const VideoDescription& desc, BaseSink* sink): BaseIO(sink), BaseReader(sink), SyncHelper(sink->frameTime()), desc(desc) {
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
        options.set_allow_sck_capturer(true);
        options.set_allow_sck_system_picker(true);
#elif IS_LINUX
        options.set_allow_pipewire(true);
#endif
        return webrtc::DesktopCapturer::CreateGenericCapturer(options);
    }

    void DesktopCapturerModule::OnCaptureResult(const webrtc::DesktopCapturer::Result result, const std::unique_ptr<webrtc::DesktopFrame> frame) {
        if (!status) return;
        if (result == webrtc::DesktopCapturer::Result::SUCCESS) {
            const int width = frame->size().width();
            const int height = frame->size().height();

            const int chromaWidth = (width + 1) / 2;
            const int chromaHeight = (height + 1) / 2;
            const auto ySize = width * height;
            const auto uvSize = chromaWidth * chromaHeight;
            const auto yPlane = std::make_unique<uint8_t[]>(ySize);
            const auto uPlane = std::make_unique<uint8_t[]>(uvSize);
            const auto vPlane = std::make_unique<uint8_t[]>(uvSize);
            libyuv::ARGBToI420(
                frame->data(), frame->stride(),
                yPlane.get(), width,
                uPlane.get(), chromaWidth,
                vPlane.get(), chromaWidth,
                width, height
            );

            int outW = desc.width;
            int outH = desc.height;
            if (width * desc.height > desc.width * height) {
                outH = desc.width * height / width;
            } else {
                outW = desc.height * width / height;
            }
            outW &= ~1;
            outH &= ~1;
            if (outW < 2) outW = 2;
            if (outH < 2) outH = 2;

            const auto videoSink = dynamic_cast<VideoSink*>(sink);
            if (auto config = videoSink->getConfig(); config->width != outW || config->height != outH) {
                config->width = static_cast<int16_t>(outW);
                config->height = static_cast<int16_t>(outH);
                videoSink->setConfig(config);
            }

            const int outChromaW = outW / 2;
            const int outChromaH = outH / 2;
            const auto outYSize = outW * outH;
            const auto outUvSize = outChromaW * outChromaH;
            auto yuv = bytes::make_unique_binary(outYSize + outUvSize * 2);
            const auto uDst = yuv.get() + outYSize;
            const auto vDst = uDst + outUvSize;

            if (outW == width && outH == height) {
                std::memcpy(yuv.get(), yPlane.get(), outYSize);
                std::memcpy(uDst, uPlane.get(), outUvSize);
                std::memcpy(vDst, vPlane.get(), outUvSize);
            } else {
                I420Scale(
                    yPlane.get(), width,
                    uPlane.get(), chromaWidth,
                    vPlane.get(), chromaWidth,
                    width, height,
                    yuv.get(), outW,
                    uDst, outChromaW,
                    vDst, outChromaW,
                    outW, outH,
                    libyuv::kFilterBox
                );
            }

            (void) dataCallback(std::move(yuv), {
                0,
                webrtc::kVideoRotation_0,
                static_cast<uint16_t>(outW),
                static_cast<uint16_t>(outH),
            });
        } else if (result == webrtc::DesktopCapturer::Result::ERROR_PERMANENT) {
            (void) eofCallback();
        }
    }

    void DesktopCapturerModule::OnSelection() {}

    void DesktopCapturerModule::OnCancelled() {
        (void) eofCallback();
    }

    void DesktopCapturerModule::OnError() {
        (void) eofCallback();
    }


    std::vector<DeviceInfo> DesktopCapturerModule::GetSources() {
#ifdef IS_MACOS
        const json metadata{
            {"id", 0},
            {"display_id", 0}
        };
        return {DeviceInfo("Screen / Window", metadata.dump())};
#else
        const auto capturer = CreateCapturer();
        if (!capturer) {
            throw MediaDeviceError("Failed to create desktop capturer");
        }
        webrtc::DesktopCapturer::SourceList sources;
        capturer->GetSourceList(&sources);
        std::vector<DeviceInfo> devices;
        for (const auto& [id, title, display_id] : sources) {
            const json metadata{
                {"id", id},
                {"display_id", display_id}
            };
            devices.emplace_back(title.empty() ? "Screen" : title, metadata.dump());
        }
        return devices;
#endif
    }

    void DesktopCapturerModule::open() {
        if (running) return;
        running = true;
        GLibLoopManager::AddInstance();
        capturer->Start(this);
#ifdef IS_MACOS
        if (const auto controller = capturer->GetDelegatedSourceListController()) {
            controller->Observe(this);
            controller->EnsureVisible();
        }
#endif
        capturer->CaptureFrame();
        thread = webrtc::PlatformThread::SpawnJoinable(
            [this] {
                while (running) {
                    waitNextFrame();
                    capturer->CaptureFrame();
                }
            },
            "DesktopCapturerModule",
            webrtc::ThreadAttributes().SetPriority(webrtc::ThreadPriority::kRealtime)
        );
    }

    bool DesktopCapturerModule::IsSupported() {
        return CreateCapturer() != nullptr;
    }
} // ntgcalls

#endif