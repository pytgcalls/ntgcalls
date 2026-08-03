//
// Created by Lauren on 28/09/24.
//

#include <ranges>
#include <ntgcalls/exceptions.hpp>
#include <ntgcalls/io/threaded_reader.hpp>
#include <ntgcalls/media/audio_receiver.hpp>
#include <ntgcalls/media/audio_sink.hpp>
#include <ntgcalls/media/audio_streamer.hpp>
#include <ntgcalls/media/base_receiver.hpp>
#include <ntgcalls/media/media_source_factory.hpp>
#include <ntgcalls/media/stream_manager.hpp>
#include <ntgcalls/media/video_receiver.hpp>
#include <ntgcalls/media/video_sink.hpp>
#include <ntgcalls/media/video_streamer.hpp>
#include <rtc_base/logging.h>

namespace ntgcalls::media {
    StreamManager::StreamManager(wrtc::utils::SafeThread& worker_thread): worker_thread_(worker_thread) {}

    void StreamManager::close() {
        std::vector<std::unique_ptr<io::BaseReader>> readers_to_close;
        {
            const std::lock_guard lock(mutex_);
            if (detached_) return;
            {
                const std::lock_guard sync_lock(sync_mutex_);
                sync_readers_.clear();
            }
            sync_cv_.notify_all();
            on_eof_ = nullptr;
            frames_callback_ = nullptr;
            on_change_status_ = nullptr;
            for (auto& reader : readers_ | std::views::values) {
                readers_to_close.push_back(std::move(reader));
            }
            readers_.clear();
            writers_.clear();
            for (const auto& stream : streams_ | std::views::values) {
                if (const auto audio_receiver = dynamic_cast<AudioReceiver*>(stream.get())) {
                    audio_receiver->on_frames(nullptr);
                } else if (const auto video_receiver = dynamic_cast<VideoReceiver*>(stream.get())) {
                    video_receiver->on_frame(nullptr);
                }
            }
            streams_.clear();
            tracks_.clear();
        }
        for (auto& reader : readers_to_close) {
            reader->on_data(nullptr);
            reader->on_eof(nullptr);
            reader.reset();
        }
    }

    void StreamManager::enable_video_simulcast(const bool enable) {
       video_simulcast_ = enable;
    }

    void StreamManager::set_stream_sources(const Mode mode, const MediaDescription& desc) {
        RTC_LOG(LS_VERBOSE) << "Setting Configuration, Acquiring lock";
        const std::lock_guard lock(mutex_);
        RTC_LOG(LS_VERBOSE) << "Setting Configuration, Lock acquired";

        const bool was_idling = is_paused();

        maybe_reconfigure_device<AudioSink, AudioDescription>(mode, Microphone, desc.microphone);
        maybe_reconfigure_device<AudioSink, AudioDescription>(mode, Speaker, desc.speaker);

        const bool was_camera = has_device_internal(mode, Camera);
        const bool was_screen = has_device_internal(mode, Screen);

        if (!video_simulcast_ && desc.camera && desc.screen && mode == Capture) {
            throw InvalidParams("Cannot mix camera and screen sources");
        }

        maybe_reconfigure_device<VideoSink, VideoDescription>(mode, Camera, desc.camera);
        maybe_reconfigure_device<VideoSink, VideoDescription>(mode, Screen, desc.screen);

        if (mode == Capture && (was_camera != has_device_internal(mode, Camera) || was_screen != has_device_internal(mode, Screen) || was_idling) && initialized_) {
            check_upgrade();
        }
    }

    void StreamManager::optimize_sources(wrtc::interfaces::NetworkInterface* pc) {
        pc->enable_audio_incoming(writers_.contains(Microphone) || external_writers_.contains(Microphone));
        pc->enable_video_incoming(writers_.contains(Camera) || external_writers_.contains(Camera), false);
        pc->enable_video_incoming(writers_.contains(Screen) || external_writers_.contains(Screen), true);
        initialized_ = pc->get_connection_mode() != wrtc::ConnectionMode::None;
    }

    MediaState StreamManager::get_state() {
        const std::lock_guard lock(mutex_);
        bool muted = false;
        for (const auto& [key, track] : tracks_) {
            if (key.first != Capture) {
                continue;
            }
            if (!track->enabled()) {
                muted = true;
                break;
            }
        }
        const auto paused = is_paused();
        return MediaState{
            muted,
            (paused || muted),
            !has_device_internal(Capture, Camera),
            (paused || muted),
            !has_device_internal(Capture, Screen),
        };
    }

    void StreamManager::detach() {
        const std::lock_guard lock(mutex_);
        tracks_.clear();
        detached_ = initialized_;
        resume_on_reconnect_ = !is_paused();
        initialized_ = false;
        for (const auto& reader : readers_ | std::views::values) {
            reader->set_enabled(false);
        }
    }

    bool StreamManager::pause() {
        return update_pause(true);
    }

    bool StreamManager::resume() {
        return update_pause(false);
    }

    bool StreamManager::mute() {
        return update_mute(true);
    }

    bool StreamManager::unmute() {
        return update_mute(false);
    }

    uint64_t StreamManager::time(const Mode mode) {
        const std::lock_guard lock(mutex_);
        uint64_t average_time = 0;
        int count = 0;
        for (const auto& [key, stream] : streams_) {
            if (stream->time() == 0 || key.first != mode) {
                continue;
            }
            average_time += stream->time();
            count++;
        }
        if (count == 0) {
            return 0;
        }
        return average_time / count;
    }

    StreamManager::Status StreamManager::status(const Mode mode) {
        const std::lock_guard lock(mutex_);
        if (mode == Capture) {
            return readers_.empty() ? Idling : is_paused() ? Paused : Active;
        }
        return writers_.empty() ? Idling : Active;
    }

    void StreamManager::on_stream_end(const std::function<void(Type, Device)>& callback) {
        on_eof_ = callback;
    }

    void StreamManager::on_upgrade(const std::function<void(MediaState)>& callback) {
        on_change_status_ = callback;
    }

    void StreamManager::add_track(Mode mode, Device device, wrtc::interfaces::NetworkInterface* pc) {
        const StreamId id(mode, device);
        if (mode == Capture) {
            tracks_[id] = pc->add_outgoing_track(dynamic_cast<BaseStreamer*>(streams_[id].get())->createTrack());
        } else {
            if (id.second == Microphone || id.second == Speaker) {
                pc->add_incoming_audio_track(dynamic_cast<AudioReceiver*>(streams_[id].get())->remote_sink());
            } else {
                pc->add_incoming_video_track(dynamic_cast<VideoReceiver*>(streams_[id].get())->remote_sink(), id.second == Screen);
            }
        }
    }

    void StreamManager::start() {
        const std::lock_guard lock(mutex_);
        for (const auto& reader : readers_ | std::views::values) {
            reader->open();
        }
        for (const auto& writer : writers_ | std::views::values) {
            writer->open();
        }
        detached_ = false;
        if (resume_on_reconnect_) {
            resume_on_reconnect_ = false;
            const auto now = std::chrono::steady_clock::now();
            for (const auto& reader : readers_ | std::views::values) {
                if (reader->set_enabled(true)) {
                    if (const auto sync = dynamic_cast<wrtc::utils::SyncHelper*>(reader.get())) {
                        sync->synchronize_time(now);
                    }
                }
            }
        }
    }

    bool StreamManager::has_device(const Mode mode, const Device device) {
        const std::lock_guard lock(mutex_);
        return has_device_internal(mode, device);
    }

    bool StreamManager::has_readers() {
        const std::lock_guard lock(mutex_);
        return !readers_.empty();
    }

    void StreamManager::on_frames(const std::function<void(Mode, Device, const std::vector<wrtc::models::Frame>&)>& callback) {
        frames_callback_ = callback;
    }

    void StreamManager::send_external_frame(Device device, const bytes::binary& data, const wrtc::models::FrameData frame_data) {
        const StreamId id(Capture, device);
        if (!external_readers_.contains(device) || !streams_.contains(id)) {
            throw InvalidParams("External source not initialized");
        }
        if (const auto stream = dynamic_cast<BaseStreamer*>(streams_[id].get())) {
            const auto unique_data = bytes::make_unique_binary(data.size());
            std::memcpy(unique_data.get(), data.data(), data.size());
            stream->sendData(unique_data.get(), data.size(), frame_data);
        }
    }

    bool StreamManager::update_mute(const bool is_muted) {
        const std::lock_guard lock(mutex_);
        bool changed = false;
        for (const auto& [key, track] : tracks_) {
            if (key.first == Playback || key.second == Camera || key.second == Screen) {
                continue;
            }
            changed |= track->set_enabled(!is_muted);
        }
        if (changed) {
            check_upgrade();
        }
        return changed;
    }

    bool StreamManager::update_pause(const bool is_paused) {
        const std::lock_guard lock(mutex_);
        auto changed = false;
        const auto now = std::chrono::steady_clock::now();
        for (const auto& reader : readers_ | std::views::values) {
            changed |= reader->set_enabled(!is_paused);
        }
        if (changed) {
            if (!is_paused) {
                for (const auto& reader : readers_ | std::views::values) {
                    if (const auto threaded_reader = dynamic_cast<wrtc::utils::SyncHelper*>(reader.get())) {
                        threaded_reader->synchronize_time(now);
                    }
                }
            }
            check_upgrade();
        }
        return changed;
    }

    bool StreamManager::is_paused() {
        if (!initialized_) {
            return false;
        }
        auto res = false;
        for (const auto& reader : readers_ | std::views::values) {
            if (!reader->is_enabled()) {
                res = true;
            }
        }
        return res;
    }

    bool StreamManager::has_device_internal(const Mode mode, const Device device) const {
        if (mode == Capture) {
            return readers_.contains(device) || external_readers_.contains(device);
        }
        return writers_.contains(device) || external_writers_.contains(device);
    }

    StreamManager::Type StreamManager::get_stream_type(const Device device) {
        switch (device) {
        case Microphone:
        case Speaker:
            return Audio;
        case Camera:
        case Screen:
            return Video;
        default:
            RTC_LOG(LS_ERROR) << "Invalid device kind";
            throw InvalidParams("Invalid device kind");
        }
    }

    void StreamManager::remove_reader(const Device device) {
        bool was_syncing;
        {
            const std::lock_guard sync_lock(sync_mutex_);
            was_syncing = sync_readers_.contains(device);
            if (was_syncing) {
                sync_readers_.erase(device);
                cancel_sync_readers_.insert(device);
            }
        }
        if (was_syncing) {
            sync_cv_.notify_all();
        }
        std::unique_ptr<io::BaseReader> reader_to_destroy;
        if (readers_.contains(device)) {
            reader_to_destroy = std::move(readers_[device]);
            readers_.erase(device);
        }
        if (reader_to_destroy) {
            mutex_.unlock();
            reader_to_destroy->on_data(nullptr);
            reader_to_destroy->on_eof(nullptr);
            reader_to_destroy.reset();
            mutex_.lock();
        }
        external_readers_.erase(device);
        {
            const std::lock_guard sync_lock(sync_mutex_);
            cancel_sync_readers_.erase(device);
        }
    }

    void StreamManager::check_upgrade() {
        const std::weak_ptr weak(shared_from_this());
        worker_thread_.PostTask([weak] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
           (void) strong->on_change_status_(strong->get_state());
        });
    }

    template<typename SinkType, typename DescriptionType>
    void StreamManager::maybe_reconfigure_device(Mode mode, Device device, const std::optional<DescriptionType> &desc) {
        const StreamId id(
            mode,
            device
        );

        const auto stream_type = get_stream_type(device);

        if (!streams_.contains(id)) {
            if (mode == Capture) {
                if (stream_type == Audio) {
                    streams_[id] = std::make_unique<AudioStreamer>();
                } else {
                    streams_[id] = std::make_unique<VideoStreamer>();
                }
            } else {
                if (stream_type == Audio) {
                    streams_[id] = std::make_unique<AudioReceiver>();
                } else {
                    streams_[id] = std::make_unique<VideoReceiver>();
                }
                dynamic_cast<BaseReceiver*>(streams_[id].get())->open();
            }
        }

        if (!desc) {
            if (detached_) {
                return;
            }
            handle_no_description(mode, device);
            return;
        }

        const auto& d = desc.value();
        const auto is_external = d.media_source == DescriptionType::MediaSource::External;
        const auto reason = detect_reconfigure_reason<SinkType, DescriptionType>(id, d, is_external);

        if (reason == ReconfigureReason::None) {
            return;
        }

        switch (mode) {
        case Capture:
            handle_capture_config(id, d, reason, stream_type, is_external);
            break;
        case Playback:
            handle_playback_config(id, d, reason, stream_type, is_external);
            break;
        }
    }

    void StreamManager::handle_no_description(const Mode mode, const Device device) {
        if (mode == Capture) {
            remove_reader(device);
            return;
        }
        writers_.erase(device);
        external_writers_.erase(device);
    }

    template<typename SinkType, typename DescriptionType>
    StreamManager::ReconfigureReason StreamManager::detect_reconfigure_reason(
        const StreamId& id,
        const DescriptionType& desc,
        const bool is_external
    ) {
        auto* sink = dynamic_cast<SinkType*>(streams_[id].get());
        if (sink && sink->set_config(desc)) {
            return ReconfigureReason::SinkConfigChanged;
        }
        if (id.first == Capture) {
            if (!readers_.contains(id.second)) {
                return ReconfigureReason::ReaderMissing;
            }
        } else {
            if (!writers_.contains(id.second)) {
                return ReconfigureReason::WriterMissing;
            }
        }
        if (is_external && !external_writers_.contains(id.second)) {
            return ReconfigureReason::ExternalStateChanged;
        }
        return ReconfigureReason::None;
    }

    template<typename DescriptionType>
    void StreamManager::handle_capture_config(
        const StreamId &id,
        const DescriptionType &desc,
        ReconfigureReason reason,
        const Type stream_type,
        const bool is_external
    ) {
        const auto& device = id.second;
        RTC_LOG(LS_INFO) << "Reconfiguring CAPTURE for device " << device << " reason=" << static_cast<int>(reason);
        const bool is_shared = desc.media_source == DescriptionType::MediaSource::Device;

        remove_reader(device);

        if (is_external) {
            external_readers_.insert(device);
            {
                const std::lock_guard sync_lock(sync_mutex_);
                sync_readers_.insert(device);
            }
            return;
        }

        readers_[device] = MediaSourceFactory::from_input(desc, streams_[id].get());

        setup_capture_callbacks(id, stream_type, is_shared);

        if (initialized_) {
            readers_[device]->open();
            RTC_LOG(LS_VERBOSE) << "Reader opened";
        }
    }

    void StreamManager::setup_capture_callbacks(const StreamId &id, Type stream_type, bool is_shared) {
        const auto& device = id.second;
        const std::weak_ptr weak(shared_from_this());

        readers_[device]->on_data([weak, id, stream_type, is_shared](const bytes::unique_binary& data, wrtc::models::FrameData frame_data) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            {
                std::unique_lock lock(strong->sync_mutex_);
                if (strong->sync_readers_.contains(id.second)) {
                    strong->sync_readers_.erase(id.second);
                    strong->sync_cv_.notify_all();
                    strong->sync_cv_.wait(lock, [strong, id] {
                        return strong->sync_readers_.empty() || strong->cancel_sync_readers_.contains(id.second);
                    });
                    if (strong->cancel_sync_readers_.contains(id.second)) {
                        strong->cancel_sync_readers_.erase(id.second);
                        return;
                    }
                    if (const auto threaded_reader = dynamic_cast<wrtc::utils::SyncHelper*>(strong->readers_[id.second].get())) {
                        threaded_reader->synchronize_time();
                    }
                }
            }
            std::vector<wrtc::models::Frame> frames_to_emit;
            {
                const std::lock_guard lock(strong->mutex_);
                if (strong->streams_.contains(id)) {
                    const auto frame_size = strong->streams_[id]->frame_size();
                    if (const auto stream = dynamic_cast<BaseStreamer*>(strong->streams_[id].get())) {
                        frame_data.absolute_capture_timestamp_ms = webrtc::TimeMillis();
                        if (stream_type == Video && is_shared) {
                            frames_to_emit.push_back({0, {data.get(), data.get() + frame_size}, frame_data});
                        }
                        stream->sendData(data.get(), frame_size, frame_data);
                    }
                }
            }
            if (!frames_to_emit.empty()) {
                (void) strong->frames_callback_(id.first, id.second, std::move(frames_to_emit));
            }
        });

        readers_[device]->on_eof([weak, device] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            strong->worker_thread_.PostTask([weak, device] {
                const auto strong_thread = weak.lock();
                if (!strong_thread) {
                    return;
                }
                {
                    const std::lock_guard lock(strong_thread->mutex_);
                    strong_thread->remove_reader(device);
                }
                (void) strong_thread->on_eof_(get_stream_type(device), device);
            });
        });
    }

    template<typename DescriptionType>
    void StreamManager::handle_playback_config(
        const StreamId &id,
        const DescriptionType &desc,
        ReconfigureReason reason,
        const Type stream_type,
        const bool is_external
    ) {
        const auto& device = id.second;
        RTC_LOG(LS_INFO) << "Reconfiguring PLAYBACK for device " << device << " reason=" << static_cast<int>(reason);

        if (is_external) {
            external_writers_.insert(device);
        }

        if (stream_type == Audio) {
            if (!is_external) {
                writers_.erase(device);
                writers_[device] = MediaSourceFactory::from_audio_output(desc, streams_[id].get());
            }
            setup_audio_playback_callbacks(id, is_external);
        } else if (is_external) {
            setup_video_playback_callbacks(id);
        } else {
            throw InvalidParams("Invalid input mode");
        }

        if (!is_external) {
            if (initialized_) {
                writers_[device]->open();
            }
        }
    }


    void StreamManager::setup_audio_playback_callbacks(const StreamId &id, bool is_external) {
        const std::weak_ptr weak(shared_from_this());
        dynamic_cast<AudioReceiver*>(streams_[id].get())->on_frames([weak, id, is_external](const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            if (is_external) {
                std::vector<wrtc::models::Frame> external_frames;
                for (const auto& [ssrc, data] : frames) {
                    if (strong->external_writers_.contains(id.second)) {
                        external_frames.push_back({
                            ssrc,
                            {data.first.get(), data.first.get() + data.second},
                            {}
                        });
                    }
                }
                (void) strong->frames_callback_(
                    id.first,
                    id.second,
                    external_frames
                );
            } else {
                if (strong->writers_.contains(id.second)) {
                    if (const auto audio_writer = dynamic_cast<io::AudioWriter*>(strong->writers_[id.second].get())) {
                        audio_writer->send_frames(frames);
                    }
                }
            }
        });
    }

    void StreamManager::setup_video_playback_callbacks(const StreamId &id) {
        const std::weak_ptr weak(shared_from_this());
        dynamic_cast<VideoReceiver*>(streams_[id].get())->on_frame([weak, id](const uint32_t ssrc, const bytes::unique_binary& frame, const size_t size, const wrtc::models::FrameData frame_data) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            if (strong->external_writers_.contains(id.second)) {
                (void) strong->frames_callback_(
                    id.first,
                    id.second,
                    {
                        {
                            ssrc,
                            {frame.get(), frame.get() + size},
                            frame_data
                        }
                    }
                );
            }
        });
    }

} // ntgcalls::media