//
// Created by Laky64 on 28/09/24.
//

#pragma once

#include <condition_variable>
#include <wrtc/wrtc.hpp>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/io/base_writer.hpp>
#include <ntgcalls/media/base_sink.hpp>
#include <ntgcalls/models/media_description.hpp>
#include <ntgcalls/models/media_state.hpp>
#include <wrtc/interfaces/network_interface.hpp>

namespace ntgcalls {

    class StreamManager: public std::enable_shared_from_this<StreamManager> {
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

        struct CallInfo {
            Status playback, capture;
        };

        enum Mode {
            Capture,
            Playback,
        };

        enum Device {
            Microphone,
            Speaker,
            Camera,
            Screen,
        };

        void close();

        void enableVideoSimulcast(bool enable);

        void setStreamSources(Mode mode, const MediaDescription& desc = MediaDescription());

        void optimizeSources(wrtc::NetworkInterface* pc);

        MediaState getState();

        bool pause();

        bool resume();

        bool mute();

        bool unmute();

        uint64_t time(Mode mode);

        Status status(Mode mode);

        void onStreamEnd(const std::function<void(Type, Device)> &callback);

        void onUpgrade(const std::function<void(MediaState)> &callback);

        void addTrack(Mode mode, Device device, wrtc::NetworkInterface* pc);

        void start();

        bool hasDevice(Mode mode, Device device);

        bool hasReaders();

        void onFrames(const std::function<void(Mode, Device, const std::vector<wrtc::Frame>&)>& callback);

        void sendExternalFrame(Device device, const bytes::binary& data, wrtc::FrameData frameData);

    private:
        bool initialized = false, videoSimulcast = true;
        std::map<std::pair<Mode, Device>, std::unique_ptr<BaseSink>> streams;
        std::map<std::pair<Mode, Device>, std::unique_ptr<wrtc::MediaTrackInterface>> tracks;
        std::map<Device, std::unique_ptr<BaseReader>> readers;
        std::map<Device, std::unique_ptr<BaseWriter>> writers;
        std::set<Device> externalWriters;
        std::set<Device> externalReaders;
        std::mutex syncMutex;
        std::condition_variable syncCV;
        std::set<Device> syncReaders, cancelSyncReaders;
        std::mutex mutex;
        wrtc::synchronized_callback<Type, Device> onEOF;
        wrtc::synchronized_callback<MediaState> onChangeStatus;
        wrtc::synchronized_callback<Mode, Device, std::vector<wrtc::Frame>> framesCallback;

        template<typename SinkType, typename DescriptionType>
        void setConfig(Mode mode, Device device, const std::optional<DescriptionType>& desc);

        void checkUpgrade();

        bool updateMute(bool isMuted);

        bool updatePause(bool isPaused);

        bool isPaused();

        bool hasDeviceInternal(Mode mode, Device device) const;

        static Type getStreamType(Device device);
    };
} // ntgcalls
