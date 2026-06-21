//
// Created by Laky64 on 15/03/2024.
//
#pragma once
#include <ntgcalls/instances/p2p_call.hpp>
#include <ntgcalls/instances/call_interface.hpp>
#include <wrtc/models/media_content.hpp>
#include <wrtc/utils/json.hpp>
#include <wrtc/interfaces/group_connection.hpp>

namespace ntgcalls {
    using wrtc::json;

    class GroupCall: public CallInterface {
        wrtc::synchronized_callback<void()> broadcastTimestampCallback;
        wrtc::synchronized_callback<void(wrtc::SegmentPartRequest)> segmentPartRequestCallback;

        static void updateRemoteVideoConstraints(const wrtc::GroupConnection* conn);
    protected:
        std::shared_ptr<wrtc::GroupConnection> presentationConnection;

    public:
        explicit GroupCall(wrtc::SafeThread& updateThread): CallInterface(updateThread) {}

        void stop() override;

        std::string init();

        virtual std::string initPresentation();

        virtual void connect(const std::string& jsonData, bool isPresentation);

        uint32_t addIncomingVideo(int64_t userID, const std::string& endpoint, const std::vector<wrtc::SsrcGroup>& ssrcGroup) const;

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
