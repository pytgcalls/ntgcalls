//
// Created by Lauren on 07/10/24.
//

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include <wrtc/interfaces/media/remote_media_interface.hpp>
#include <wrtc/models/audio_frame.hpp>

namespace wrtc::interfaces::media {

    class RemoteAudioSink final: public RemoteMediaInterface, public std::enable_shared_from_this<RemoteAudioSink> {
        std::atomic<uint32_t> num_sources_;
        std::atomic_bool encoded_ = false;
        std::atomic<int64_t> max_delay_ms_;
        std::vector<std::unique_ptr<models::AudioFrame>> audio_frames_;
        std::function<void(const std::vector<std::unique_ptr<models::AudioFrame>>&)> frames_callback_;

    public:
        explicit RemoteAudioSink(const std::function<void(const std::vector<std::unique_ptr<models::AudioFrame>>&)>& callback);

        ~RemoteAudioSink() override;

        void send_data(std::unique_ptr<models::AudioFrame> frame);

        void update_audio_source_count(int count);

        void set_encoded(bool encoded);

        [[nodiscard]] bool encoded() const;

        void set_max_delay(std::optional<uint32_t> max_delay_ms);

        [[nodiscard]] int64_t max_delay_ms() const;
    };

} // wrtc::interfaces::media
