//
// Created by Laky64 on 07/10/24.
//

#pragma once

#include <atomic>
#include <memory>
#include <wrtc/models/audio_frame.hpp>
#include <wrtc/utils/syncronized_callback.hpp>
#include <wrtc/interfaces/media/remote_media_interface.hpp>

namespace wrtc {

    class RemoteAudioSink final: public RemoteMediaInterface {
        std::atomic<uint32_t> numSources;
        std::vector<std::shared_ptr<AudioFrame>> audioFrames;
        synchronized_callback<std::vector<std::shared_ptr<AudioFrame>>> framesCallback;

    public:
        explicit RemoteAudioSink(const std::function<void(const std::vector<std::shared_ptr<AudioFrame>>&)>& callback);

        ~RemoteAudioSink() override;

        void sendData(const std::shared_ptr<AudioFrame>& frame);

        void addSource();

        void removeSource();
    };

} // wrtc
