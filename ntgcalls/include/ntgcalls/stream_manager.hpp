//
// Created by Laky64 on 28/09/24.
//

#pragma once

#include <shared_mutex>
#include <wrtc/wrtc.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/media/base_sink.hpp>
#include <ntgcalls/models/media_description.hpp>
#include <ntgcalls/models/media_state.hpp>

namespace ntgcalls {

    class StreamManager {
    public:
        enum Type {
            Audio,
            Video,
        };

        enum Status {
            Active,
            Paused,
            Idling,
        };

        struct MediaStatus {
            Status playback, recording;
        };

        enum Direction {
            Input,
            Output,
        };

        enum Device {
            Microphone,
            Speaker,
            Camera,
            Screen,
        };

        explicit StreamManager(rtc::Thread* workerThread);

        ~StreamManager();

        void setStreamSources(Direction direction, const MediaDescription& desc);

        MediaState getState();

        bool pause();

        bool resume();

        bool mute();

        bool unmute();

        uint64_t time(Direction direction);

        Status status(Direction direction);

        void onStreamEnd(const std::function<void(Type, Device)> &callback);

        void onUpgrade(const std::function<void(MediaState)> &callback);

        void addTrack(Direction direction, Device device, const std::unique_ptr<wrtc::NetworkInterface> &pc);

        void start();

    private:
        rtc::Thread* workerThread;
        bool idling = false, initialized = false, hasVideo = false;
        std::map<std::pair<Direction, Device>, std::unique_ptr<BaseSink>> streams;
        std::map<std::pair<Direction, Device>, std::unique_ptr<wrtc::MediaTrackInterface>> tracks;
        std::map<Device, std::unique_ptr<BaseReader>> readers;
        std::shared_mutex mutex;
        wrtc::synchronized_callback<Type, Device> onEOF;
        wrtc::synchronized_callback<MediaState> onChangeStatus;

        template<typename SinkType, typename DescriptionType>
        void setConfig(Direction direction, Device device, const std::optional<DescriptionType>& desc);

        void checkUpgrade();

        bool updateMute(bool isMuted);

        static Type getStreamType(Device device);
    };
} // ntgcalls
