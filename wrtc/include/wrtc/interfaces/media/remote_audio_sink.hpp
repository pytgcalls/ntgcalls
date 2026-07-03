//
// Created by Lauren on 07/10/24.
//

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>
#include <wrtc/interfaces/media/remote_media_interface.hpp>
#include <wrtc/models/audio_frame.hpp>

namespace wrtc::interfaces::media {

    class RemoteAudioSink final: public RemoteMediaInterface, public std::enable_shared_from_this<RemoteAudioSink> {
        std::atomic<uint32_t> num_sources_;
        std::vector<std::unique_ptr<models::AudioFrame>> audio_frames_;
        std::function<void(const std::vector<std::unique_ptr<models::AudioFrame>>&)> frames_callback_;

    public:
        explicit RemoteAudioSink(const std::function<void(const std::vector<std::unique_ptr<models::AudioFrame>>&)>& callback);

        ~RemoteAudioSink() override;

        void send_data(std::unique_ptr<models::AudioFrame> frame);

        void add_source();

        void remove_source();

        void update_audio_source_count(int count);
    };

} // wrtc::interfaces::media
