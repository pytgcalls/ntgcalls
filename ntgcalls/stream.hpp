//
// Created by Laky64 on 12/08/2023.
//

#pragma once


#include "media/audio_streamer.hpp"
#include "media/video_streamer.hpp"
#include "utils/dispatch_queue.hpp"
#include "io/base_reader.hpp"
#include "configs.hpp"

namespace ntgcalls {
    class Stream {
    private:
        std::shared_ptr<AudioStreamer> audio;
        std::shared_ptr<VideoStreamer> video;
        std::shared_ptr<BaseReader> is_audio, is_video;
        bool running = false, idling = false, lipSync = false;
        DispatchQueue dispatchQueue = DispatchQueue("StreamQueue");

        void sendSample();

        std::pair<std::shared_ptr<BaseStreamer>, std::shared_ptr<BaseReader>> unsafePrepareForSample();

    public:
        Stream();

        ~Stream();

        void setAVStream(StreamConfig streamConfig);

        void start();

        void pause();

        void resume();

        void stop();

        void addTracks(const std::shared_ptr<wrtc::PeerConnection> &pc);
    };
}