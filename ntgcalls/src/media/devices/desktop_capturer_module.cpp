//
// Created by Lauren on 15/10/24.
//

#ifndef IS_ANDROID
#include <modules/desktop_capture/desktop_capturer_differ_wrapper.h>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/media/video_sink.hpp>
#include <ntgcalls/media/devices/desktop_capturer_module.hpp>
#include <ntgcalls/utils/g_lib_loop_manager.hpp>
#include <third_party/libyuv/include/libyuv.h>

namespace ntgcalls::media::devices {
    DesktopCapturerModule::DesktopCapturerModule(const VideoDescription& desc, BaseSink* sink): BaseIO(sink), BaseReader(sink), SyncHelper(sink->frame_time()), desc_(desc) {
        capturer_ = create_capturer();
        try {
            auto source_metadata = json::parse(desc.input);
            capturer_->SelectSource(source_metadata["id"].get<webrtc::DesktopCapturer::SourceId>());
        } catch (...) {
            throw MediaDeviceError("Invalid device metadata");
        }
        capturer_->SetMaxFrameRate(desc.fps);
    }

    DesktopCapturerModule::~DesktopCapturerModule() {
        running_ = false;
        thread_.Finalize();
        utils::GLibLoopManager::remove_instance();
    }

    std::unique_ptr<webrtc::DesktopCapturer> DesktopCapturerModule::create_capturer() {
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
        if (!status_) return;
        if (result == webrtc::DesktopCapturer::Result::SUCCESS) {
            const int width = frame->size().width();
            const int height = frame->size().height();

            const int chroma_width = (width + 1) / 2;
            const int chroma_height = (height + 1) / 2;
            const auto y_size = width * height;
            const auto uv_size = chroma_width * chroma_height;
            const auto y_plane = std::make_unique<uint8_t[]>(y_size);
            const auto u_plane = std::make_unique<uint8_t[]>(uv_size);
            const auto v_plane = std::make_unique<uint8_t[]>(uv_size);
            libyuv::ARGBToI420(
                frame->data(), frame->stride(),
                y_plane.get(), width,
                u_plane.get(), chroma_width,
                v_plane.get(), chroma_width,
                width, height
            );

            int out_w = desc_.width;
            int out_h = desc_.height;
            if (width * desc_.height > desc_.width * height) {
                out_h = desc_.width * height / width;
            } else {
                out_w = desc_.height * width / height;
            }
            out_w &= ~1;
            out_h &= ~1;
            if (out_w < 2) out_w = 2;
            if (out_h < 2) out_h = 2;

            const auto video_sink = dynamic_cast<VideoSink*>(sink_);
            if (auto config = video_sink->get_config(); config->width != out_w || config->height != out_h) {
                config->width = static_cast<int16_t>(out_w);
                config->height = static_cast<int16_t>(out_h);
                video_sink->set_config(config);
            }

            const int out_chroma_w = out_w / 2;
            const int out_chroma_h = out_h / 2;
            const auto out_y_size = out_w * out_h;
            const auto out_uv_size = out_chroma_w * out_chroma_h;
            auto yuv = bytes::make_unique_binary(out_y_size + out_uv_size * 2);
            const auto u_dst = yuv.get() + out_y_size;
            const auto v_dst = u_dst + out_uv_size;

            if (out_w == width && out_h == height) {
                std::memcpy(yuv.get(), y_plane.get(), out_y_size);
                std::memcpy(u_dst, u_plane.get(), out_uv_size);
                std::memcpy(v_dst, v_plane.get(), out_uv_size);
            } else {
                I420Scale(
                    y_plane.get(), width,
                    u_plane.get(), chroma_width,
                    v_plane.get(), chroma_width,
                    width, height,
                    yuv.get(), out_w,
                    u_dst, out_chroma_w,
                    v_dst, out_chroma_w,
                    out_w, out_h,
                    libyuv::kFilterBox
                );
            }

            (void) data_callback_(std::move(yuv), {
                0,
                webrtc::kVideoRotation_0,
                static_cast<uint16_t>(out_w),
                static_cast<uint16_t>(out_h),
            });
        } else if (result == webrtc::DesktopCapturer::Result::ERROR_PERMANENT) {
            (void) eof_callback_();
        }
    }

    void DesktopCapturerModule::OnSelection() {}

    void DesktopCapturerModule::OnCancelled() {
        (void) eof_callback_();
    }

    void DesktopCapturerModule::OnError() {
        (void) eof_callback_();
    }


    std::vector<DeviceInfo> DesktopCapturerModule::get_sources() {
#ifdef IS_MACOS
        const json metadata{
            {"id", 0},
            {"display_id", 0}
        };
        return {DeviceInfo("Screen / Window", metadata.dump())};
#else
        const auto capturer = create_capturer();
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
        if (running_) return;
        running_ = true;
        utils::GLibLoopManager::add_instance();
        capturer_->Start(this);
#ifdef IS_MACOS
        if (const auto controller = capturer_->GetDelegatedSourceListController()) {
            controller->Observe(this);
            controller->EnsureVisible();
        }
#endif
        capturer_->CaptureFrame();
        thread_ = webrtc::PlatformThread::SpawnJoinable(
            [this] {
                while (running_) {
                    wait_next_frame();
                    capturer_->CaptureFrame();
                }
            },
            "DesktopCapturerModule",
            webrtc::ThreadAttributes().SetPriority(webrtc::ThreadPriority::kRealtime)
        );
    }

    bool DesktopCapturerModule::is_supported() {
        return create_capturer() != nullptr;
    }
} // ntgcalls::media::devices

#endif