//
// Created by Laky64 on 15/03/2024.
//

#include <ntgcalls/instances/group_call.hpp>

#include <future>

#include <ntgcalls/exceptions.hpp>
#include <wrtc/interfaces/group_connection.hpp>
#include <wrtc/models/response_payload.hpp>

namespace ntgcalls {
    GroupCall::~GroupCall() {
        stopPresentation();
    }

    std::string GroupCall::init(const MediaDescription& config) {
        RTC_LOG(LS_INFO) << "Initializing group call";
        if (connection) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        connection = std::make_shared<wrtc::GroupConnection>(false);
        connection->open();
        RTC_LOG(LS_INFO) << "Group call initialized";
        streamManager->setStreamSources(StreamManager::Mode::Capture, config);
        streamManager->setStreamSources(StreamManager::Mode::Playback, MediaDescription());
        streamManager->optimizeSources(connection.get());

        connection->onDataChannelOpened([this] {
            RTC_LOG(LS_INFO) << "Data channel opened";
            updateRemoteVideoConstraints();
        });
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Microphone, connection.get());
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Camera, connection.get());
        streamManager->addTrack(StreamManager::Mode::Playback, StreamManager::Device::Microphone, connection.get());
        streamManager->addTrack(StreamManager::Mode::Playback, StreamManager::Device::Camera, connection.get());
        streamManager->addTrack(StreamManager::Mode::Playback, StreamManager::Device::Screen, connection.get());
        RTC_LOG(LS_INFO) << "AVStream settings applied";
        return Safe<wrtc::GroupConnection>(connection)->getJoinPayload();
    }

    std::string GroupCall::initPresentation() {
        RTC_LOG(LS_INFO) << "Initializing screen sharing";
        if (presentationConnection) {
            RTC_LOG(LS_ERROR) << "Screen sharing already initialized";
            throw ConnectionError("Screen sharing already initialized");
        }
        presentationConnection = std::make_shared<wrtc::GroupConnection>(true);
        presentationConnection->open();
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Speaker, presentationConnection.get());
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Screen, presentationConnection.get());
        RTC_LOG(LS_INFO) << "Screen sharing initialized";
        return presentationConnection->getJoinPayload();
    }

    void GroupCall::connect(const std::string& jsonData, const bool isPresentation) {
        RTC_LOG(LS_INFO) << "Connecting to group call";
        const auto &conn = isPresentation ? presentationConnection : connection;
        if (!conn) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionError("Connection not initialized");
        }
        wrtc::ResponsePayload payload(jsonData);
        Safe<wrtc::GroupConnection>(conn)->setConnectionMode(payload.isRtmp ? wrtc::GroupConnection::Mode::Rtmp : wrtc::GroupConnection::Mode::Rtc);
        if (!payload.isRtmp) {
            Safe<wrtc::GroupConnection>(conn)->setRemoteParams(payload.remoteIceParameters, std::move(payload.fingerprint));
            for (const auto& rawCandidate : payload.candidates) {
                webrtc::JsepIceCandidate iceCandidate{std::string(), 0, rawCandidate};
                conn->addIceCandidate(wrtc::IceCandidate(&iceCandidate));
            }
            Safe<wrtc::GroupConnection>(conn)->createChannels(payload.media);
            RTC_LOG(LS_INFO) << "Remote parameters set";
        } else {
            RTC_LOG(LS_ERROR) << "RTMP connection not supported";
            throw RTMPNeeded("RTMP connection not supported");
        }
        setConnectionObserver(isPresentation ? CallNetworkState::Kind::Presentation : CallNetworkState::Kind::Normal);
    }

    void GroupCall::updateRemoteVideoConstraints() const {
        json jsonRes = {
            {"colibriClass", "ReceiverVideoConstraints"},
            {"constraints", json::object()},
            {"defaultConstraints", {{"maxHeight", 0}}},
            {"onStageEndpoints", json::array()}
        };
        for (const auto& endpoint : Safe<wrtc::GroupConnection>(connection)->getEndpoints()) {
            jsonRes["constraints"][endpoint] = {
                {"maxHeight", 360},
                {"minHeight", 180},
            };
        }
        connection->sendDataChannelMessage(bytes::make_binary(to_string(jsonRes)));
    }

    uint32_t GroupCall::addIncomingVideo(const std::string& endpoint, const std::vector<wrtc::SsrcGroup>& ssrcGroup) const {
        const auto ssrc = Safe<wrtc::GroupConnection>(connection)->addIncomingVideo(endpoint, ssrcGroup);
        updateRemoteVideoConstraints();
        return ssrc;
    }

    bool GroupCall::removeIncomingVideo(const std::string& endpoint) const {
        return Safe<wrtc::GroupConnection>(connection)->removeIncomingVideo(endpoint);
    }

    void GroupCall::stopPresentation(const bool force) {
        if (!force && !presentationConnection) {
            return;
        }
        if (presentationConnection) {
            presentationConnection->close();
            presentationConnection = nullptr;
        } else {
            throw ConnectionError("Presentation not initialized");
        }
    }

    void GroupCall::onUpgrade(const std::function<void(MediaState)>& callback) const {
        streamManager->onUpgrade(callback);
    }

    CallInterface::Type GroupCall::type() const {
        return Type::Group;
    }
} // ntgcalls