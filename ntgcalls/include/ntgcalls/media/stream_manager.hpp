//
// Created by Lauren on 28/09/24.
//

#pragma once

#include <condition_variable>
#include <ntgcalls/io/base_reader.hpp>
#include <ntgcalls/io/base_writer.hpp>
#include <ntgcalls/media/base_sink.hpp>
#include <ntgcalls/media/media_description.hpp>
#include <ntgcalls/media/media_state.hpp>
#include <wrtc/interfaces/network_interface.hpp>
#include <wrtc/models/frame.hpp>

namespace ntgcalls::media {
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

        explicit StreamManager(wrtc::utils::SafeThread& worker_thread);

        void close();

        void enable_video_simulcast(bool enable);

        void set_stream_sources(Mode mode, const MediaDescription& desc = MediaDescription());

        void optimize_sources(wrtc::interfaces::NetworkInterface* pc);

        MediaState get_state();

        void detach();

        bool pause();

        bool resume();

        bool mute();

        bool unmute();

        uint64_t time(Mode mode);

        Status status(Mode mode);

        void on_stream_end(const std::function<void(Type, Device)> &callback);

        void on_upgrade(const std::function<void(MediaState)> &callback);

        void add_track(Mode mode, Device device, wrtc::interfaces::NetworkInterface* pc);

        void start();

        bool has_device(Mode mode, Device device);

        bool has_readers();

        void on_frames(const std::function<void(Mode, Device, const std::vector<wrtc::models::Frame>&)>& callback);

        void send_external_frame(Device device, const bytes::binary& data, wrtc::models::FrameData frame_data);

    private:
        using StreamId = std::pair<Mode, Device>;

        wrtc::utils::SafeThread& worker_thread_;
        bool initialized_ = false, video_simulcast_ = true, resume_on_reconnect_ = false, detached_ = false;
        std::map<StreamId, std::unique_ptr<BaseSink>> streams_;
        std::map<StreamId, std::unique_ptr<wrtc::interfaces::media::tracks::MediaTrackInterface>> tracks_;
        std::map<Device, std::unique_ptr<io::BaseReader>> readers_;
        std::map<Device, std::unique_ptr<io::BaseWriter>> writers_;
        std::set<Device> external_writers_;
        std::set<Device> external_readers_;
        std::mutex sync_mutex_;
        std::condition_variable sync_cv_;
        std::set<Device> sync_readers_, cancel_sync_readers_;
        std::mutex mutex_;
        wrtc::utils::synchronized_callback<void(Type, Device)> on_eof_;
        wrtc::utils::synchronized_callback<void(MediaState)> on_change_status_;
        wrtc::utils::synchronized_callback<void(Mode, Device, std::vector<wrtc::models::Frame>)> frames_callback_;

        enum class ReconfigureReason {
            None,
            SinkConfigChanged,
            ReaderMissing,
            WriterMissing,
            ExternalStateChanged
        };

        template<typename SinkType, typename DescriptionType>
        void maybe_reconfigure_device(Mode mode, Device device, const std::optional<DescriptionType>& desc);

        template<class SinkType, class DescriptionType>
        ReconfigureReason detect_reconfigure_reason(const StreamId &id, const DescriptionType &desc, bool is_external);

        template<typename DescriptionType>
        void handle_capture_config(
            const StreamId& id,
            const DescriptionType& desc,
            ReconfigureReason reason,
            Type stream_type,
            bool is_external
        );

        void setup_capture_callbacks(
            const StreamId& id,
            Type stream_type,
            bool is_shared
        );

        template<typename DescriptionType>
        void handle_playback_config(
            const StreamId& id,
            const DescriptionType& desc,
            ReconfigureReason reason,
            Type stream_type,
            bool is_external
        );

        void setup_audio_playback_callbacks(
            const StreamId& id,
            bool is_external
        );

        void setup_video_playback_callbacks(
            const StreamId &id
        );

        void handle_no_description(Mode mode, Device device);

        void check_upgrade();

        bool update_mute(bool is_muted);

        bool update_pause(bool is_paused);

        bool is_paused();

        bool has_device_internal(Mode mode, Device device) const;

        static Type get_stream_type(Device device);

        void remove_reader(Device device);
    };
} // ntgcalls::media
