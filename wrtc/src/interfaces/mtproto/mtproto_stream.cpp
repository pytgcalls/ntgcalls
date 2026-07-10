//
// Created by Lauren on 12/04/25.
//

#include <ranges>
#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>
#include <wrtc/exceptions.hpp>
#include <wrtc/interfaces/mtproto/mtproto_stream.hpp>

namespace wrtc::interfaces::mtproto {

    MTProtoStream::MTProtoStream(utils::SafeThread& media_thread, const bool is_rtmp) : is_rtmp_(is_rtmp), media_thread_(media_thread) {}

    void MTProtoStream::connect() {
        if (running_) {
            throw RTCException("MTProto Connection already made");
        }
        running_ = true;
        server_time_ms_ = webrtc::TimeUTCMillis();
        server_time_ms_got_at_ = webrtc::TimeMillis();
        render();
    }

    void MTProtoStream::close() {
        thread_buffer_ = nullptr;
        audio_frame_callback_ = nullptr;
        video_frame_callback_ = nullptr;
        request_current_time_callback_ = nullptr;
        request_broadcast_part_callback_ = nullptr;
        update_audio_source_count_callback_ = nullptr;
        running_ = false;
    }

    void MTProtoStream::send_broadcast_timestamp(const int64_t timestamp) {
        const std::weak_ptr weak(shared_from_this());
        media_thread_.PostTask([weak, timestamp]{
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            const std::lock_guard lock(strong->segment_mutex_);
            strong->is_waiting_current_time_ = false;
            int64_t adjusted_timestamp = 0;
            if (timestamp > 0) {
                adjusted_timestamp = timestamp / strong->segment_duration_ * strong->segment_duration_ - strong->segment_buffer_duration_;
            }
            if (adjusted_timestamp <= 0) {
                const int task_id = strong->next_pending_request_time_delay_task_id_;
                strong->pending_request_time_delay_task_id_ = task_id;
                strong->next_pending_request_time_delay_task_id_++;

                strong->media_thread_.PostDelayedTask([weak, task_id] {
                    const auto strong_media = weak.lock();
                    if (!strong_media) {
                        return;
                    }
                    const std::lock_guard lock_media(strong_media->segment_mutex_);
                    if (strong_media->pending_request_time_delay_task_id_ != task_id) {
                        return;
                    }
                    strong_media->pending_request_time_delay_task_id_ = 0;
                    strong_media->request_segments_if_needed();
                }, webrtc::TimeDelta::Millis(1000));
            } else {
                strong->next_segment_timestamp_ = adjusted_timestamp;
                strong->request_segments_if_needed();
            }
        });
    }

    void MTProtoStream::send_broadcast_part(const int64_t segment_id, const int32_t part_id, const models::MediaSegment::Part::Status status, const bool quality_update, std::optional<bytes::binary> data) {
        const std::weak_ptr weak(shared_from_this());
        media_thread_.PostTask([weak, segment_id, part_id, status, quality_update, data = std::move(data)] {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }

            const std::lock_guard lock(strong->segment_mutex_);
            bool found_part = false;
            if (strong->segments_.contains(segment_id)) {
                if (quality_update) {
                    found_part = strong->segments_[segment_id]->video.size() > part_id &&
                        strong->segments_[segment_id]->video[part_id]->quality_update_part;
                } else {
                    found_part = strong->segments_[segment_id]->parts.size() > part_id;
                }
            }

            if (!found_part) {
                return;
            }

            const auto &segment = strong->segments_[segment_id];
            models::MediaSegment::Part* part;
            if (quality_update) {
                part = segment->video[part_id]->quality_update_part.get();
            } else {
                part = segment->parts[part_id].get();
            }
            const auto response_timestamp = webrtc::TimeMillis();
            const auto response_timestamp_milliseconds = static_cast<int64_t>(static_cast<double>(response_timestamp) * 1000.0);
            const auto response_timestamp_boundary = response_timestamp_milliseconds / strong->segment_duration_ * strong->segment_duration_;

            part->status = status;
            switch (status) {
            case models::MediaSegment::Part::Status::Success:
                part->data = data;
                if (strong->next_segment_timestamp_ == -1) {
                    strong->next_segment_timestamp_ = part->timestamp_milliseconds + strong->segment_duration_;
                }
                strong->check_pending_segments();
                if (quality_update) {
                    segment->video[part_id]->part = std::make_unique<VideoStreamingPart>(std::move(part->data.value()));
                    segment->video[part_id]->quality_update_part = nullptr;
                }
                break;
            case models::MediaSegment::Part::Status::NotReady:
                if (segment->timestamp == 0 && !strong->is_rtmp_) {
                    strong->next_segment_timestamp_ = response_timestamp_boundary;
                    strong->discard_all_pending_segments();
                    strong->request_segments_if_needed();
                    strong->check_pending_segments();
                } else {
                    part->min_request_timestamp = webrtc::TimeMillis() + 100;
                    strong->check_pending_segments();
                }
                break;
            case models::MediaSegment::Part::Status::ResyncNeeded:
                if (strong->is_rtmp_) {
                    strong->next_segment_timestamp_ = -1;
                } else {
                    strong->next_segment_timestamp_ = response_timestamp_boundary;
                }
                strong->discard_all_pending_segments();
                strong->request_segments_if_needed();
                strong->check_pending_segments();
                break;
            default:
                throw RTCException("Invalid part status");
            }
        });
    }

    void MTProtoStream::add_incoming_video(const std::string& endpoint, const uint32_t ssrc, bool is_screen_cast) {
        if (is_rtmp_) {
            return;
        }
        const std::lock_guard lock(segment_mutex_);
        video_channels_[endpoint] = VideoChannel(
            ssrc,
            is_screen_cast,
            is_screen_cast ? models::MediaSegment::Quality::Full : models::MediaSegment::Quality::Medium
        );
        check_pending_video_quality_update();
    }

    bool MTProtoStream::remove_incoming_video(const std::string& endpoint) {
        if (is_rtmp_) {
            return false;
        }
        const std::lock_guard lock(segment_mutex_);
        if (video_channels_.contains(endpoint)) {
            video_channels_.erase(endpoint);
            check_pending_video_quality_update();
            return true;
        }
        return false;
    }

    void MTProtoStream::enable_audio_incoming(const bool enable) {
        audio_incoming_ = enable;
    }

    void MTProtoStream::enable_video_incoming(const bool enable, const bool is_screen_cast) {
        if (is_screen_cast) {
            screen_incoming_ = enable;
        } else {
            camera_incoming_ = enable;
        }
    }

    void MTProtoStream::on_request_broadcast_time(const std::function<void()>& callback) {
        request_current_time_callback_ = callback;
    }

    void MTProtoStream::on_request_broadcast_part(const std::function<void(models::SegmentPartRequest)>& callback) {
        request_broadcast_part_callback_ = callback;
    }

    void MTProtoStream::on_audio_frame(const std::function<void(std::unique_ptr<models::AudioFrame>)>& callback){
        audio_frame_callback_ = callback;
    }

    void MTProtoStream::on_video_frame(const std::function<void(uint32_t, bool, std::unique_ptr<webrtc::VideoFrame>)>& callback) {
        video_frame_callback_ = callback;
    }

    void MTProtoStream::on_update_audio_source_count(const std::function<void(int count)>& callback) {
        update_audio_source_count_callback_ = callback;
    }

    std::map<int64_t, models::MediaSegment*> MTProtoStream::filter_segments(const models::MediaSegment::Status status) const {
        std::map<int64_t, models::MediaSegment*> available_segments;
        for (const auto& [fst, snd] : segments_) {
            if (snd->status == status) {
                available_segments[fst] = snd.get();
            }
        }
        return available_segments;
    }

    void MTProtoStream::render() {
        const std::weak_ptr weak(shared_from_this());
        thread_buffer_ = std::make_unique<ThreadBuffer>([weak] (const webrtc::MediaType media_type, models::MediaSegment* segment, const std::chrono::milliseconds relative_timestamp) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            const std::shared_lock lock(strong->segment_mutex_);
            if (media_type == webrtc::MediaType::AUDIO) {
                if ((segment->audio || segment->unified_audio) && strong->audio_incoming_) {
                    std::vector<AudioStreamingPartState::Channel> audio_channels;
                    if (strong->is_rtmp_) {
                        audio_channels = segment->unified_audio->get_audio10ms_per_channel(strong->persistent_audio_decoder_);
                    } else {
                        audio_channels = segment->audio->get_10ms_per_channel(segment->audio_decoder);
                    }

                    if (audio_channels.empty()) {
                        return;
                    }

                    std::map<uint32_t, std::unique_ptr<AudioBuffer>> interleaved_audio_by_ssrc;
                    for (const auto& [ssrc, pcmData] : audio_channels) {
                        const uint32_t real_ssrc = strong->is_rtmp_ ? 1 : ssrc;
                        const size_t sample_count = strong->is_rtmp_ ? 480 : pcmData.size();

                        auto &prev_buffer = interleaved_audio_by_ssrc[real_ssrc];
                        if (!prev_buffer) {
                            prev_buffer = std::make_unique<AudioBuffer>();
                            prev_buffer->ssrc = real_ssrc;
                            prev_buffer->sample_rate = sample_count * 100;
                        }

                        const int channel_count = prev_buffer->channels;
                        prev_buffer->channels++;

                        std::vector<int16_t> output;
                        output.reserve(sample_count * channel_count);
                        for (size_t i = 0; i < sample_count; ++i) {
                            for (int j = 0; j < channel_count; ++j) {
                                output.push_back(i < prev_buffer->data.size() ? prev_buffer->data[i] : static_cast<int16_t>(0));
                            }
                            output.push_back(i < pcmData.size() ? pcmData[i] : static_cast<int16_t>(0));
                        }
                        prev_buffer->data = std::move(output);
                    }
                    for (const auto& buffer : interleaved_audio_by_ssrc | std::views::values) {
                        auto frame = std::make_unique<models::AudioFrame>(buffer->ssrc);
                        frame->channels = buffer->channels;
                        frame->sample_rate = static_cast<int>(buffer->sample_rate);
                        frame->data = buffer->data.data();
                        frame->size = buffer->data.size() * sizeof(int16_t);
                        strong->audio_frame_callback_(std::move(frame));
                    }
                }
            } else {
                std::set<std::string> used_endpoints;
                for (const auto &video_segment : segment->video) {
                    video_segment->is_playing = true;
                    std::optional<std::string> endpoint_id;

                    if (strong->is_rtmp_) {
                        endpoint_id = "unified";
                    } else {
                        cancel_pending_video_quality_update(video_segment.get());
                        endpoint_id = video_segment->part->get_active_endpoint_id();
                    }

                    if (endpoint_id.has_value() && (strong->video_channels_.contains(endpoint_id.value()) || strong->is_rtmp_)) {
                        bool is_screen_cast = false;
                        uint32_t ssrc = 1;

                        if (!strong->is_rtmp_) {
                            const auto video_channel = strong->video_channels_[endpoint_id.value()];
                            is_screen_cast = video_channel.is_screen_cast;
                            ssrc = video_channel.ssrc;
                        }

                        if ((is_screen_cast && strong->screen_incoming_) || (!is_screen_cast && strong->camera_incoming_)) {
                            if (!strong->shared_video_state_.contains(endpoint_id.value())) {
                                strong->shared_video_state_[endpoint_id.value()] = std::make_unique<VideoStreamingSharedState>();
                            }
                            used_endpoints.insert(endpoint_id.value());
                            if (const auto frame = video_segment->part->get_frame_at_relative_timestamp(
                                strong->shared_video_state_[endpoint_id.value()].get(),
                                static_cast<double>(relative_timestamp.count()) / 1000.0
                            )) {
                                if (video_segment->last_frame_pts != frame->pts) {
                                    video_segment->last_frame_pts = frame->pts;
                                    auto video_frame = std::make_unique<webrtc::VideoFrame>(frame->frame);
                                    const auto frame_timestamp = static_cast<int64_t>(frame->pts * 1000) + segment->timestamp;
                                    video_frame->set_timestamp_us(frame_timestamp);
                                    strong->video_frame_callback_(
                                        ssrc,
                                        is_screen_cast,
                                        std::move(video_frame)
                                    );
                                }
                            }
                        }
                    }
                }

                if (!strong->is_rtmp_) {
                    for (auto it = strong->shared_video_state_.begin(); it != strong->shared_video_state_.end();) {
                        if (!used_endpoints.contains(it->first) && !strong->video_channels_.contains(it->first)) {
                            it = strong->shared_video_state_.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        },
        [weak] () -> models::MediaSegment* {
            const auto strong = weak.lock();
            if (!strong) {
                return nullptr;
            }
            const std::lock_guard lock(strong->segment_mutex_);
            if (strong->wait_for_buffered_milliseconds_before_rendering_) {
                if (strong->get_available_buffer_duration() < strong->wait_for_buffered_milliseconds_before_rendering_.value()) {
                    return nullptr;
                }
                strong->wait_for_buffered_milliseconds_before_rendering_ = std::nullopt;
            }
            const auto available_segments = strong->filter_segments(models::MediaSegment::Status::Ready);
            if (available_segments.empty()) {
                strong->wait_for_buffered_milliseconds_before_rendering_ = strong->segment_buffer_duration_ + strong->segment_duration_;
                return nullptr;
            }
            return available_segments.begin()->second;
        },
        [weak] (const ThreadBuffer::RequestType request_type) {
            const auto strong = weak.lock();
            if (!strong) {
                return;
            }
            const std::lock_guard lock(strong->segment_mutex_);
            switch (request_type) {
            case ThreadBuffer::RequestType::RequestSegments:
                strong->request_segments_if_needed();
                strong->check_pending_segments();
                break;
            case ThreadBuffer::RequestType::RemoveSegment:
                const auto segment = strong->segments_.begin();
                if (segment == strong->segments_.end()) {
                    return;
                }
                strong->segments_.erase(segment);
                break;
            }
        });
    }

    int64_t MTProtoStream::get_available_buffer_duration() const {
        int64_t result = 0;
        for (const auto segment : filter_segments(models::MediaSegment::Status::Ready) | std::views::values) {
            result += segment->duration;
        }
        return result;
    }

    void MTProtoStream::request_segments_if_needed() {
        while (true) {
            if (next_segment_timestamp_ == -1) {
                if (!is_waiting_current_time_ && pending_request_time_delay_task_id_ == 0) {
                    is_waiting_current_time_ = true;
                    if (is_rtmp_) {
                        (void) request_current_time_callback_();
                    } else {
                        send_broadcast_timestamp(server_time_ms_ + (webrtc::TimeMillis() - server_time_ms_got_at_));
                    }
                }
                break;
            }
            int64_t available_and_requested_segments_duration = 0;
            available_and_requested_segments_duration += get_available_buffer_duration();
            available_and_requested_segments_duration += static_cast<int64_t>(filter_segments(models::MediaSegment::Status::Pending).size()) * segment_duration_;

            if (available_and_requested_segments_duration > segment_buffer_duration_) {
                break;
            }

            auto pending_segment = std::make_unique<models::MediaSegment>();
            pending_segment->timestamp = next_segment_timestamp_;

            if (next_segment_timestamp_ != -1) {
                next_segment_timestamp_ += segment_duration_;
            }

            std::unique_ptr<models::MediaSegment::Part> audio_part;
            if (is_rtmp_) {
                audio_part = std::make_unique<models::MediaSegment::Part>(models::MediaSegment::Part::Unified());
            } else {
                audio_part = std::make_unique<models::MediaSegment::Part>(models::MediaSegment::Part::Audio());
            }
            pending_segment->parts.push_back(std::move(audio_part));

            for (const auto &[endpoint, videoChannel] : video_channels_) {
                if (!current_endpoint_mapping_.contains(endpoint)) {
                    continue;
                }
                const int32_t channel_id = current_endpoint_mapping_[endpoint] + 1;
                auto video_part = std::make_unique<models::MediaSegment::Part>(models::MediaSegment::Part::Video(channel_id, videoChannel.quality));
                pending_segment->parts.push_back(std::move(video_part));
            }
            if (segments_.contains(next_segment_timestamp_)) {
                return;
            }
            segments_[next_segment_timestamp_] = std::move(pending_segment);

            if (next_segment_timestamp_ == -1) {
                break;
            }
        }
    }

    void MTProtoStream::check_pending_segments() {
        if (!running_) {
            return;
        }
        const auto absolute_timestamp = webrtc::TimeMillis();
        int64_t min_delayed_request_timeout = INT_MAX;

        bool should_request_more_segments = false;
        int i = 0;
        for (auto& [segmentID, pendingSegment] : filter_segments(models::MediaSegment::Status::Pending)) {
            const auto segment_timestamp = pendingSegment->timestamp;
            bool all_parts_done = true;
            for (int part_id = 0; part_id < pendingSegment->parts.size(); part_id++) {
                const auto part = pendingSegment->parts[part_id].get();
                if (!part->data) {
                    all_parts_done = false;
                }
                if (!part->data && part->status != models::MediaSegment::Part::Status::Downloading) {
                    if (part->min_request_timestamp != 0) {
                        if (part->min_request_timestamp > absolute_timestamp) {
                            min_delayed_request_timeout = std::min(min_delayed_request_timeout, part->min_request_timestamp - absolute_timestamp);
                            continue;
                        }
                    }
                    auto video_quality = models::MediaSegment::Quality::None;
                    int32_t video_channel_id = 0;
                    const auto type_data = &part->type_data;

                    if (const auto video = std::get_if<models::MediaSegment::Part::Video>(type_data)) {
                        video_quality = video->quality;
                        video_channel_id = video->channel_id;
                    } else if (std::get_if<models::MediaSegment::Part::Unified>(type_data)) {
                        video_quality = models::MediaSegment::Quality::Full;
                        video_channel_id = 1;
                    }

                    const auto requested = request_broadcast_part_callback_({
                        segmentID,
                        part_id,
                        models::SegmentPartRequest::kDefaultSize,
                        segment_timestamp,
                        false,
                        video_channel_id,
                        video_quality
                    });

                    if (requested) {
                        part->status = models::MediaSegment::Part::Status::Downloading;
                        part->timestamp_milliseconds = segment_timestamp;
                    }
                }
            }

            if (all_parts_done && i == 0) {
                pendingSegment->duration = segment_duration_;
                pendingSegment->status = models::MediaSegment::Status::Ready;
                for (const auto& part : pendingSegment->parts) {
                    if (const auto type_data = &part->type_data; std::get_if<models::MediaSegment::Part::Audio>(type_data)) {
                        pendingSegment->audio = std::make_unique<AudioStreamingPart>(std::move(part->data.value()), "ogg", false);
                        current_endpoint_mapping_ = pendingSegment->audio->get_endpoint_mapping();
                    } else if (const auto video_data = std::get_if<models::MediaSegment::Part::Video>(type_data)) {
                        auto video_segment = std::make_unique<models::MediaSegment::Video>();
                        video_segment->quality = video_data->quality;
                        if (part->data.value().empty()) {
                            RTC_LOG(LS_VERBOSE) << "Video part " << pendingSegment->timestamp << " is empty";
                        }
                        video_segment->part = std::make_unique<VideoStreamingPart>(std::move(part->data.value()));
                        pendingSegment->video.push_back(std::move(video_segment));
                    } else if (std::get_if<models::MediaSegment::Part::Unified>(type_data)) {
                        auto unified_segment = std::make_unique<models::MediaSegment::Video>();
                        bytes::binary data_copy = part->data.value();
                        unified_segment->part = std::make_unique<VideoStreamingPart>(std::move(part->data.value()));
                        pendingSegment->video.push_back(std::move(unified_segment));
                        pendingSegment->unified_audio = std::make_unique<VideoStreamingPart>(std::move(data_copy), webrtc::MediaType::AUDIO);
                    }
                }
                pendingSegment->parts.clear();
                should_request_more_segments = true;
                i--;
            }
            i++;
        }

        if (min_delayed_request_timeout < INT32_MAX) {
            const std::weak_ptr weak(shared_from_this());
            media_thread_.PostDelayedTask([weak] {
                const auto strong = weak.lock();
                if (!strong) {
                    return;
                }
                const std::lock_guard lock(strong->segment_mutex_);
                strong->check_pending_segments();
            }, webrtc::TimeDelta::Millis(std::max(static_cast<int32_t>(min_delayed_request_timeout), 10)));
        }

        if (should_request_more_segments) {
            request_segments_if_needed();
        }
    }

    void MTProtoStream::discard_all_pending_segments() {
        for (auto it = segments_.begin(); it != segments_.end(); ) {
            if (it->second->status == models::MediaSegment::Status::Pending) {
                it = segments_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void MTProtoStream::cancel_pending_video_quality_update(models::MediaSegment::Video* segment) {
        if (!segment->quality_update_part) {
            return;
        }
        segment->quality_update_part = nullptr;
    }

    void MTProtoStream::check_pending_video_quality_update() {
        for (const auto & [endpointId, videoChannel] : video_channels_) {
            for (const auto &[segment_id, segment] : filter_segments(models::MediaSegment::Status::Ready)) {
                for (int part_id = 0; part_id < segment->video.size(); part_id++) {
                    if (const auto video = segment->video[part_id].get(); video->part->get_active_endpoint_id() == endpointId) {
                        if (video->quality != videoChannel.quality) {
                            request_pending_video_quality_update(segment_id, part_id, video, segment->timestamp);
                        }
                    }
                }
            }
        }
    }

    void MTProtoStream::request_pending_video_quality_update(int64_t segment_id, int32_t part_id, models::MediaSegment::Video* segment, int64_t timestamp) {
        if (segment->is_playing) {
            return;
        }

        if (const auto segment_endpoint_id = segment->part->get_active_endpoint_id(); !segment_endpoint_id) {
            return;
        }

        std::optional<int32_t> updated_channel_id;
        std::optional<models::MediaSegment::Quality> updated_quality;

        for (const auto & [endpointId, videoChannel] : video_channels_) {
            if (!current_endpoint_mapping_.contains(endpointId)) {
                continue;
            }

            updated_channel_id = current_endpoint_mapping_[endpointId] + 1;
            updated_quality = videoChannel.quality;
        }

        if (updated_channel_id && updated_quality) {
            if (segment->quality_update_part) {
                const auto type_data = &segment->quality_update_part->type_data;
                if (const auto video_data = std::get_if<models::MediaSegment::Part::Video>(type_data)) {
                    if (video_data->channel_id == updated_channel_id.value() && video_data->quality == updated_quality.value()) {
                        return;
                    }
                }
                cancel_pending_video_quality_update(segment);
            }

            auto video = std::make_unique<models::MediaSegment::Part>(
                models::MediaSegment::Part::Video(updated_channel_id.value(), updated_quality.value())
            );
            video->status = models::MediaSegment::Part::Status::Downloading;
            video->min_request_timestamp = 0;
            segment->quality_update_part = std::move(video);

            (void) request_broadcast_part_callback_({
                segment_id,
                part_id,
                models::SegmentPartRequest::kDefaultSize,
                timestamp,
                true,
                updated_channel_id.value(),
                updated_quality.value()
            });
        }
    }

} // wrtc::interfaces::mtproto