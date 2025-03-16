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
        std::map<std::string, bool> endpointsKind;
        std::map<std::string, std::vector<wrtc::SsrcGroup>> pendingIncomingPresentations;

        static void updateRemoteVideoConstraints(const std::shared_ptr<wrtc::NetworkInterface>& conn) ;

    public:
        explicit GroupCall(rtc::Thread* updateThread): CallInterface(updateThread) {}

        ~GroupCall() override;

        std::string init(const MediaDescription& config);

        std::string initPresentation();

        void connect(const std::string& jsonData, bool isPresentation);

        uint32_t addIncomingVideo(const std::string& endpoint, const std::vector<wrtc::SsrcGroup>& ssrcGroup);

        bool removeIncomingVideo(const std::string& endpoint);

        void stopPresentation(bool force = false);

        void setStreamSources(StreamManager::Mode mode, const MediaDescription& config) const override;

        Type type() const override;

        void onUpgrade(const std::function<void(MediaState)> &callback) const;
    };

} // ntgcalls
