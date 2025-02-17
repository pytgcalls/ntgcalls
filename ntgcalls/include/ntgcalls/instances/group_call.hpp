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

        void updateRemoteVideoConstraints() const;

    public:
        explicit GroupCall(rtc::Thread* updateThread): CallInterface(updateThread) {}

        ~GroupCall() override;

        std::string init(const MediaDescription& config);

        std::string initPresentation();

        void connect(const std::string& jsonData, bool isPresentation);

        uint32_t addIncomingVideo(const std::string& endpoint, const std::vector<wrtc::SsrcGroup>& ssrcGroup) const;

        bool removeIncomingVideo(const std::string& endpoint) const;

        void stopPresentation(bool force = false);

        Type type() const override;

        void onUpgrade(const std::function<void(MediaState)> &callback) const;
    };

} // ntgcalls
