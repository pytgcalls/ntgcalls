//
// Created by Laky64 on 15/03/2024.
//
#pragma once
#include <ntgcalls/instances/call_interface.hpp>
#include <wrtc/models/media_content.hpp>
#include <nlohmann/json.hpp>
#include <wrtc/interfaces/group_connection.hpp>

namespace ntgcalls {
    using json = nlohmann::json;

    class GroupCall final : public CallInterface {
        std::shared_ptr<wrtc::GroupConnection> presentationConnection;
        wrtc::synchronized_callback<void> broadcastTimestampCallback;
        wrtc::synchronized_callback<wrtc::SegmentPartRequest> segmentPartRequestCallback;

        static void updateRemoteVideoConstraints(const wrtc::GroupConnection* conn) ;

    public:
        explicit GroupCall(webrtc::Thread* updateThread): CallInterface(updateThread) {}

        void stop() override;

        std::string init();

        std::string initPresentation();

        void connect(const std::string& jsonData, bool isPresentation);

        uint32_t addIncomingVideo(const std::string& endpoint, const std::vector<wrtc::SsrcGroup>& ssrcGroup) const;

        bool removeIncomingVideo(const std::string& endpoint) const;

        void stopPresentation(bool force = false);

        void setStreamSources(StreamManager::Mode mode, const MediaDescription& config) const override;

        Type type() const override;

        void onUpgrade(const std::function<void(MediaState)> &callback) const;

        void sendBroadcastPart(int64_t segmentID, int32_t partID, wrtc::MediaSegment::Part::Status status, bool qualityUpdate, const std::optional<bytes::binary>& data) const;

        void onRequestedBroadcastPart(const std::function<void(wrtc::SegmentPartRequest)>& callback);

        void sendBroadcastTimestamp(int64_t timestamp) const;

        void onRequestedBroadcastTimestamp(const std::function<void()>& callback);
    };

} // ntgcalls
