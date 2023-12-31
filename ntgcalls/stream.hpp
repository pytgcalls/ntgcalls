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

        void setAVStream(const MediaDescription& streamConfig, bool noUpgrade = false);

        void start();

        bool pause();

        bool resume();

        bool mute() const;

        bool unmute() const;

        void stop();

        MediaState getState() const;

        uint64_t time() const;

        Status status() const;

        void addTracks(const std::shared_ptr<wrtc::PeerConnection> &pc);

        void onStreamEnd(const std::function<void(Type)> &callback);

        void onUpgrade(const std::function<void(MediaState)> &callback);

    private:
        std::shared_ptr<AudioStreamer> audio;
        std::shared_ptr<VideoStreamer> video;
        wrtc::MediaStreamTrack *audioTrack{}, *videoTrack{};
        std::shared_ptr<MediaReaderFactory> reader;
        bool running = false, idling = false, changing = false, hasVideo = false;
        wrtc::synchronized_callback<Type> onEOF;
        wrtc::synchronized_callback<MediaState> onChangeStatus;
        std::shared_ptr<DispatchQueue> streamQueue;
        std::shared_ptr<DispatchQueue> updateQueue;
        std::recursive_mutex mutex;

        void sendSample();

        void checkStream() const;

        std::pair<std::shared_ptr<BaseStreamer>, std::shared_ptr<BaseReader>> unsafePrepareForSample() const;

        void checkUpgrade() const;
    };
}