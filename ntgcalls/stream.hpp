//
// Created by Laky64 on 12/08/2023.
//

#pragma once


#include "io/base_reader.hpp"
#include "models/media_state.hpp"
#include "media/audio_streamer.hpp"
#include "media/video_streamer.hpp"
#include "utils/dispatch_queue.hpp"
#include "models/media_description.hpp"
#include "media/media_reader_factory.hpp"

namespace ntgcalls {
    class Stream {
    public:
        enum Type {
            Audio,
            Video,
        };
        enum Status {
            Playing,
            Paused,
            Idling,
        };

        Stream();

        ~Stream();

        void setAVStream(MediaDescription streamConfig, bool noUpgrade = false);

        void start();

        bool pause();

        bool resume();

        bool mute();

        bool unmute();

        void stop();

        MediaState getState();

        uint64_t time();

        Status status();

        void addTracks(const std::shared_ptr<wrtc::PeerConnection> &pc);

        void onStreamEnd(std::function<void(Stream::Type)> &callback);

        void onUpgrade(std::function<void(MediaState)> &callback);

    private:
        std::shared_ptr<AudioStreamer> audio;
        std::shared_ptr<VideoStreamer> video;
        wrtc::MediaStreamTrack *audioTrack, *videoTrack;
        std::shared_ptr<MediaReaderFactory> reader;
        bool running = false, idling = false, hasVideo = false;
        wrtc::synchronized_callback<Type> onEOF;
        wrtc::synchronized_callback<MediaState> onChangeStatus;
        std::shared_ptr<DispatchQueue> streamQueue;
        std::shared_ptr<DispatchQueue> updateQueue;

        void sendSample();

        void checkStream();

        std::pair<std::shared_ptr<BaseStreamer>, std::shared_ptr<BaseReader>> unsafePrepareForSample();

        void checkUpgrade();
    };
}