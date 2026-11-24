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
        std::map<Device, std::unique_ptr<io::BaseWriter>> writers_to_close;
        std::map<StreamId, std::unique_ptr<BaseSink>> streams_to_close;
        std::map<StreamId, std::unique_ptr<wrtc::interfaces::media::tracks::MediaTrackInterface>> tracks_to_close;
        {
            const std::lock_guard lock(mutex_);
            closed_ = true;
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
            for (auto& reader : removed_readers_ | std::views::values) {
                readers_to_close.push_back(std::move(reader));
            }
            readers_.clear();
            removed_readers_.clear();
            for (const auto& stream : streams_ | std::views::values) {
                if (const auto audio_receiver = dynamic_cast<AudioReceiver*>(stream.get())) {
                    audio_receiver->on_frames(nullptr);
                } else if (const auto video_receiver = dynamic_cast<VideoReceiver*>(stream.get())) {
                    video_receiver->on_frame(nullptr);
                    video_receiver->on_source_removed(nullptr);
                }
            }
            {
                const std::unique_lock lock(writers_mutex_);
                writers_to_close.swap(writers_);
            }
            streams_to_close.swap(streams_);
            tracks_to_close.swap(tracks_);
        }
        for (auto& reader : readers_to_close) {
            reader->on_data(nullptr);
            reader->on_eof(nullptr);
            reader.reset();
        }
        writers_to_close.clear();
        streams_to_close.clear();
        tracks_to_close.clear();
    }

    void StreamManager::enable_video_simulcast(const bool enable) {
        video_simulcast_ = enable;
    }

    void StreamManager::set_stream_sources(const Mode mode, const MediaDescription& desc) {
        if (!video_simulcast_ && desc.camera && desc.screen && mode == Capture) {
            throw InvalidParams("Cannot mix camera and screen sources");
        }
        {
            RTC_LOG(LS_VERBOSE) << "Setting Configuration, Acquiring lock";
            const std::lock_guard lock(mutex_);
            RTC_LOG(LS_VERBOSE) << "Setting Configuration, Lock acquired";
            if (closed_) {
                RTC_LOG(LS_WARNING) << "Stream sources ignored, the call is closed";
                return;
            }

            maybe_reconfigure_device<AudioSink, AudioDescription>(mode, Microphone, desc.microphone);
            maybe_reconfigure_device<AudioSink, AudioDescription>(mode, Speaker, desc.speaker);
            maybe_reconfigure_device<VideoSink, VideoDescription>(mode, Camera, desc.camera);
            maybe_reconfigure_device<VideoSink, VideoDescription>(mode, Screen, desc.screen);

            if (mode == Capture && initialized_) {
                check_upgrade();
            }
        }
        destroy_removed_readers();
    }

    void StreamManager::optimize_sources(wrtc::interfaces::NetworkInterface* pc) {
        const auto connected = pc->get_connection_mode() != wrtc::ConnectionMode::None;
        bool audio, camera, screen, was_initialized;
        {
            const std::lock_guard lock(mutex_);
            if (closed_) {
                return;
            }
            audio = has_device_internal(Playback, Microphone);
            camera = has_device_internal(Playback, Camera);
            screen = has_device_internal(Playback, Screen);
            was_initialized = initialized_;
            initialized_ = connected;
        }
        pc->enable_audio_incoming(audio);
        pc->enable_video_incoming(camera, false);
        pc->enable_video_incoming(screen, true);
        if (!was_initialized && connected) {
            const auto state = get_state();
            const std::lock_guard lock(state_mutex_);
            last_state_ = state;
        }
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
        detached_ = true;
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
            return readers_.empty() ? Idling : is_paused() ? Paused
                                                           : Active;
        }
        return writers_.empty() ? Idling : Active;
    }

    void StreamManager::on_stream_end(const std::function<void(Type, Device)>& callback) {
        on_eof_ = callback;
    }

    void StreamManager::on_upgrade(const std::function<void(MediaState)>& callback) {
        {
            const std::lock_guard lock(state_mutex_);
            last_state_.reset();
        }
        on_change_status_ = callback;
    }

    wrtc::models::AudioEncoding StreamManager::audio_encoding(const StreamId& id, const AudioDescription& description) {
        std::shared_ptr<wrtc::utils::encoded::EncodedQueue> passthrough;
        if (description.is_encoded()) {
            passthrough = dynamic_cast<AudioStreamer*>(streams_[id].get())->encoded_queue();
        }
        return {description.bitrate, description.channel_count, passthrough};
    }

    wrtc::models::VideoEncoding StreamManager::video_encoding(const StreamId& id, const VideoDescription& description) {
        std::shared_ptr<wrtc::utils::encoded::EncodedQueue> passthrough;
        if (description.is_encoded()) {
            passthrough = dynamic_cast<VideoStreamer*>(streams_[id].get())->encoded_queue();
        }
        return {passthrough};
    }

    void StreamManager::update_encoding(const StreamId& id) {
        const auto track = tracks_.find(id);
        if (track == tracks_.end()) {
            return;
        }
        if (const auto sink = dynamic_cast<AudioSink*>(streams_[id].get())) {
            if (const auto config = sink->get_config()) {
                if (const auto audio_track = dynamic_cast<wrtc::interfaces::media::tracks::EncodedTrackInterface<wrtc::models::AudioEncoding>*>(track->second.get())) {
                    audio_track->set_encoding(audio_encoding(id, *config));
                }
            }
        }
        if (const auto sink = dynamic_cast<VideoSink*>(streams_[id].get())) {
            if (const auto config = sink->get_config()) {
                if (const auto video_track = dynamic_cast<wrtc::interfaces::media::tracks::EncodedTrackInterface<wrtc::models::VideoEncoding>*>(track->second.get())) {
                    video_track->set_encoding(video_encoding(id, *config));
                }
            }
        }
    }

    void StreamManager::add_track(Mode mode, Device device, wrtc::interfaces::NetworkInterface* pc) {
        const StreamId id(mode, device);
        const std::lock_guard lock(mutex_);
        const auto stream = streams_.find(id);
        if (closed_ || stream == streams_.end()) {
            return;
        }
        if (mode == Capture) {
            tracks_[id] = pc->add_outgoing_track(dynamic_cast<BaseStreamer*>(stream->second.get())->create_track());
            update_encoding(id);
        } else {
            if (id.second == Microphone || id.second == Speaker) {
                pc->add_incoming_audio_track(dynamic_cast<AudioReceiver*>(stream->second.get())->remote_sink());
            } else {
                pc->add_incoming_video_track(dynamic_cast<VideoReceiver*>(stream->second.get())->remote_sink(), id.second == Screen);
            }
        }
    }

    void StreamManager::start() {
        const std::lock_guard lock(mutex_);
        for (const auto& [device, reader] : readers_) {
            try {
                reader->open();
            } catch (const std::exception& e) {
                RTC_LOG(LS_ERROR) << "Failed to open the reader for device " << device << ": " << e.what();
            }
        }
        for (const auto& [device, writer] : writers_) {
            try {
                writer->open();
            } catch (const std::exception& e) {
                RTC_LOG(LS_ERROR) << "Failed to open the writer for device " << device << ": " << e.what();
            }
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

    void StreamManager::send_external_frame(Device device, const bytes::binary& data, const wrtc::models::FrameData& frame_data) {
        const StreamId id(Capture, device);
        const std::lock_guard lock(mutex_);
        if (!external_readers_.contains(device) || !streams_.contains(id)) {
            throw InvalidParams("External source not initialized");
        }
        if (const auto stream = dynamic_cast<BaseStreamer*>(streams_[id].get())) {
            const auto unique_data = bytes::make_unique_binary(data.size());
            std::memcpy(unique_data.get(), data.data(), data.size());
            stream->send_data(unique_data.get(), data.size(), frame_data);
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
        if (readers_.contains(device)) {
            removed_readers_.emplace_back(device, std::move(readers_[device]));
            readers_.erase(device);
        } else if (was_syncing) {
            const std::lock_guard sync_lock(sync_mutex_);
            cancel_sync_readers_.erase(device);
        }
        external_readers_.erase(device);
    }

    void StreamManager::destroy_removed_readers() {
        std::vector<std::pair<Device, std::unique_ptr<io::BaseReader>>> removed;
        {
            const std::lock_guard lock(mutex_);
            removed = std::move(removed_readers_);
            removed_readers_.clear();
        }
        for (auto& reader : removed | std::views::values) {
            reader->on_data(nullptr);
            reader->on_eof(nullptr);
            reader.reset();
        }
        const std::lock_guard sync_lock(sync_mutex_);
        for (const auto& device : removed | std::views::keys) {
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
            strong->emit_state_if_changed();
        });
    }

    void StreamManager::emit_state_if_changed() {
        const auto state = get_state();
        {
            const std::lock_guard lock(state_mutex_);
            if (last_state_ && same_state(*last_state_, state)) {
                return;
            }
            last_state_ = state;
        }
        (void) on_change_status_(state);
    }

    bool StreamManager::same_state(const MediaState& a, const MediaState& b) {
        return a.muted == b.muted &&
               a.video_paused == b.video_paused &&
               a.video_stopped == b.video_stopped &&
               a.presentation_paused == b.presentation_paused &&
               a.presentation_stopped == b.presentation_stopped;
    }

    template<typename SinkType, typename DescriptionType>
    void StreamManager::maybe_reconfigure_device(Mode mode, Device device, const std::optional<DescriptionType>& desc) {
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
            update_encoding(id);
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
        replace_writer(device, nullptr);
        const std::unique_lock lock(writers_mutex_);
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
        const StreamId& id,
        const DescriptionType& desc,
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

    void StreamManager::setup_capture_callbacks(const StreamId& id, Type stream_type, bool is_shared) {
        const auto& device = id.second;
        const std::weak_ptr weak(shared_from_this());
        auto* reader = readers_[device].get();

        reader->on_data([weak, id, stream_type, is_shared, reader](const bytes::unique_binary& data, const size_t size, wrtc::models::FrameData frame_data) {
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
                    if (const auto threaded_reader = dynamic_cast<wrtc::utils::SyncHelper*>(reader)) {
                        threaded_reader->synchronize_time();
                    }
                }
            }
            std::vector<wrtc::models::Frame> frames_to_emit;
            {
                const std::lock_guard lock(strong->mutex_);
                if (strong->streams_.contains(id)) {
                    if (const auto stream = dynamic_cast<BaseStreamer*>(strong->streams_[id].get())) {
                        frame_data.absolute_capture_timestamp_ms = webrtc::TimeMillis();
                        if (stream_type == Video && is_shared) {
                            frames_to_emit.push_back({0, {data.get(), data.get() + size}, frame_data});
                        }
                        stream->send_data(data.get(), size, frame_data);
                    }
                }
            }
            if (!frames_to_emit.empty()) {
                (void) strong->frames_callback_(id.first, id.second, std::move(frames_to_emit));
            }
        });

        reader->on_eof([weak, device] {
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
                strong_thread->destroy_removed_readers();
                (void) strong_thread->on_eof_(get_stream_type(device), device);
            });
        });
    }

    template<typename DescriptionType>
    void StreamManager::handle_playback_config(
        const StreamId& id,
        const DescriptionType& desc,
        ReconfigureReason reason,
        const Type stream_type,
        const bool is_external
    ) {
        const auto& device = id.second;
        RTC_LOG(LS_INFO) << "Reconfiguring PLAYBACK for device " << device << " reason=" << static_cast<int>(reason);

        if (is_external) {
            const std::unique_lock lock(writers_mutex_);
            external_writers_.insert(device);
        }

        if (stream_type == Audio) {
            if constexpr (std::is_same_v<DescriptionType, AudioDescription>) {
                dynamic_cast<AudioReceiver*>(streams_[id].get())->set_encoded(desc.is_encoded());
                dynamic_cast<AudioReceiver*>(streams_[id].get())->set_max_delay(desc.max_delay_ms);
            }
            if (!is_external) {
                replace_writer(device, MediaSourceFactory::from_output(desc, streams_[id].get()));
            }
            setup_audio_playback_callbacks(id, is_external);
        } else {
            if constexpr (std::is_same_v<DescriptionType, VideoDescription>) {
                dynamic_cast<VideoReceiver*>(streams_[id].get())->set_encoded(desc.is_encoded());
            }
            if (!is_external) {
                replace_writer(device, MediaSourceFactory::from_output(desc, streams_[id].get()));
            }
            setup_video_playback_callbacks(id, is_external);
        }

        if (!is_external) {
            if (initialized_) {
                writers_[device]->open();
            }
        }
    }

    void StreamManager::setup_audio_playback_callbacks(const StreamId& id, bool is_external) {
        const std::weak_ptr weak(shared_from_this());
        auto* receiver = dynamic_cast<AudioReceiver*>(streams_[id].get());
        receiver->on_frames([weak, id, is_external, receiver](const std::map<uint32_t, std::pair<bytes::unique_binary, size_t>>& frames, const std::map<uint32_t, int64_t>& timestamps) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            std::vector<wrtc::models::Frame> converted_frames;
            for (const auto& [ssrc, data] : frames) {
                wrtc::models::FrameData frame_data{};
                frame_data.codec = receiver->codec();
                if (const auto it = timestamps.find(ssrc); it != timestamps.end()) {
                    frame_data.absolute_capture_timestamp_ms = it->second;
                }
                converted_frames.push_back({
                    ssrc,
                    {data.first.get(), data.first.get() + data.second},
                    frame_data,
                });
            }
            if (is_external) {
                if (strong->has_external_writer(id.second) && !converted_frames.empty()) {
                    (void) strong->frames_callback_(
                        id.first,
                        id.second,
                        converted_frames
                    );
                }
            } else {
                strong->send_to_writer(id.second, std::move(converted_frames));
            }
        });
    }

    void StreamManager::setup_video_playback_callbacks(const StreamId& id, bool is_external) {
        const std::weak_ptr weak(shared_from_this());
        dynamic_cast<VideoReceiver*>(streams_[id].get())->on_frame([weak, id, is_external](const uint32_t ssrc, bytes::binary frame, const wrtc::models::FrameData& frame_data) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            std::vector<wrtc::models::Frame> frames;
            frames.emplace_back(ssrc, std::move(frame), frame_data);
            if (is_external) {
                if (strong->has_external_writer(id.second)) {
                    (void) strong->frames_callback_(id.first, id.second, frames);
                }
            } else {
                strong->send_to_writer(id.second, std::move(frames));
            }
        });
        dynamic_cast<VideoReceiver*>(streams_[id].get())->on_source_removed([weak, id](const uint32_t ssrc) {
            if (const auto strong = weak.lock()) {
                strong->remove_writer_source(id.second, ssrc);
            }
        });
    }

    bool StreamManager::has_external_writer(const Device device) {
        const std::shared_lock lock(writers_mutex_);
        return external_writers_.contains(device);
    }

    void StreamManager::send_to_writer(const Device device, std::vector<wrtc::models::Frame> frames) {
        const std::shared_lock lock(writers_mutex_);
        if (const auto writer = writers_.find(device); writer != writers_.end()) {
            writer->second->send_frames(std::move(frames));
        }
    }

    void StreamManager::remove_writer_source(const Device device, const int64_t ssrc) {
        const std::shared_lock lock(writers_mutex_);
        if (const auto writer = writers_.find(device); writer != writers_.end()) {
            writer->second->remove_source(ssrc);
        }
    }

    void StreamManager::replace_writer(const Device device, std::unique_ptr<io::BaseWriter> writer) {
        std::unique_ptr<io::BaseWriter> previous;
        {
            const std::unique_lock lock(writers_mutex_);
            if (const auto it = writers_.find(device); it != writers_.end()) {
                previous = std::move(it->second);
                writers_.erase(it);
            }
            if (writer) {
                writers_[device] = std::move(writer);
            }
        }
    }

} // ntgcalls::media
