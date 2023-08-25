//
// Created by Laky64 on 12/08/2023.
//

#pragma once


#include "io/base_reader.hpp"
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

        Stream();

        ~Stream();

        void setAVStream(MediaDescription streamConfig);

        void start();

        bool pause();

        bool resume();

        bool mute();

        bool unmute();

        void stop();

        uint64_t time();

        void addTracks(const std::shared_ptr<wrtc::PeerConnection> &pc);

        void onStreamEnd(std::function<void(Stream::Type)> &callback);

    private:
        std::shared_ptr<AudioStreamer> audio;
        std::shared_ptr<VideoStreamer> video;
        wrtc::MediaStreamTrack *audioTrack, *videoTrack;
        std::shared_ptr<MediaReaderFactory> reader;
        bool running = false, idling = false;
        wrtc::synchronized_callback<Type> onEOF;
        DispatchQueue dispatchQueue = DispatchQueue("StreamQueue");

        void sendSample();

        void checkStream();

        std::pair<std::shared_ptr<BaseStreamer>, std::shared_ptr<BaseReader>> unsafePrepareForSample();
    };
}