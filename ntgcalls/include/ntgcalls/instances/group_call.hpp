//
// Created by Laky64 on 15/03/2024.
//
#pragma once
#include <ntgcalls/instances/call_interface.hpp>

namespace ntgcalls {
    class GroupCall final : public CallInterface {
        std::unique_ptr<wrtc::NetworkInterface> presentationConnection;

    public:
        explicit GroupCall(rtc::Thread* updateThread): CallInterface(updateThread) {}

        ~GroupCall() override;

        std::string init(const MediaDescription& config);

        std::string initPresentation();

        void connect(const std::string& jsonData, bool isPresentation);

        void stopPresentation();

        Type type() const override;

        void onUpgrade(const std::function<void(MediaState)> &callback);
    };

} // ntgcalls
