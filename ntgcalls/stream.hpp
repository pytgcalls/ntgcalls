//
// Created by Laky64 on 12/08/2023.
//

#pragma once


#include <shared_mutex>

#include "models/media_state.hpp"
#include "media/audio_streamer.hpp"
#include "media/video_streamer.hpp"
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

        explicit Stream(rtc::Thread* workerThread);

        ~Stream();

        void setAVStream(const MediaDescription& streamConfig, bool noUpgrade = false);

        void start();

        bool pause();

        bool resume();

        bool mute();

        bool unmute();

        MediaState getState();

        uint64_t time();

        Status status();

        void addTracks(const std::unique_ptr<wrtc::NetworkInterface> &pc);

        void onStreamEnd(const std::function<void(Type)> &callback);

        void onUpgrade(const std::function<void(MediaState)> &callback);

    private:
        std::shared_ptr<AudioStreamer> audio;
        std::shared_ptr<VideoStreamer> video;
        wrtc::MediaStreamTrack *audioTrack{}, *videoTrack{};
        std::unique_ptr<MediaReaderFactory> reader;
        bool idling = false;
        std::atomic_bool hasVideo = false, changing = false, quit = false;
        wrtc::synchronized_callback<Type> onEOF;
        wrtc::synchronized_callback<MediaState> onChangeStatus;
        std::thread thread;
        rtc::Thread* workerThread;
        std::shared_mutex mutex;

        void checkStream() const;

        void checkUpgrade();

        bool updateMute(bool isMuted);
    };
}