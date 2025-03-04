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
            updateRemoteVideoConstraints(connection);
        });
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Microphone, connection.get());
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Camera, connection.get());
        streamManager->addTrack(StreamManager::Mode::Playback, StreamManager::Device::Microphone, connection.get());
        streamManager->addTrack(StreamManager::Mode::Playback, StreamManager::Device::Camera, connection.get());
        RTC_LOG(LS_INFO) << "AVStream settings applied";
        return Safe<wrtc::GroupConnection>(connection)->getJoinPayload();
    }

    std::string GroupCall::initPresentation() {
        initNetThread();
        RTC_LOG(LS_INFO) << "Initializing screen sharing";
        if (presentationConnection) {
            RTC_LOG(LS_ERROR) << "Screen sharing already initialized";
            throw ConnectionError("Screen sharing already initialized");
        }
        presentationConnection = std::make_shared<wrtc::GroupConnection>(true);
        presentationConnection->open();
        streamManager->optimizeSources(presentationConnection.get());
        presentationConnection->onDataChannelOpened([this] {
            RTC_LOG(LS_INFO) << "Data channel opened";
            updateThread->PostTask([this]{
                for (auto x = pendingIncomingPresentations; const auto& [endpoint, ssrcGroup] : x) {
                    addIncomingVideo(endpoint, ssrcGroup);
                }
            });
            updateRemoteVideoConstraints(presentationConnection);
        });
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Speaker, presentationConnection.get());
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Screen, presentationConnection.get());
        streamManager->addTrack(StreamManager::Mode::Playback, StreamManager::Device::Screen, presentationConnection.get());
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
            if (isPresentation) {
                const auto mediaConfig = Safe<wrtc::GroupConnection>(conn)->getMediaConfig();
                payload.media.audioPayloadTypes = mediaConfig.audioPayloadTypes;
                payload.media.audioRtpExtensions = mediaConfig.audioRtpExtensions;
            }
            Safe<wrtc::GroupConnection>(conn)->createChannels(payload.media);
            RTC_LOG(LS_INFO) << "Remote parameters set";
        } else {
            RTC_LOG(LS_ERROR) << "RTMP connection not supported";
            throw RTMPNeeded("RTMP connection not supported");
        }
        setConnectionObserver(
            conn,
            isPresentation ? CallNetworkState::Kind::Presentation : CallNetworkState::Kind::Normal
        );
    }

    void GroupCall::updateRemoteVideoConstraints(const std::shared_ptr<wrtc::NetworkInterface>& conn) {
        json jsonRes = {
            {"colibriClass", "ReceiverVideoConstraints"},
            {"constraints", json::object()},
            {"defaultConstraints", {{"maxHeight", 0}}},
            {"onStageEndpoints", json::array()}
        };
        for (const auto& endpoint : Safe<wrtc::GroupConnection>(conn)->getEndpoints()) {
            jsonRes["constraints"][endpoint] = {
                {"maxHeight", 360},
                {"minHeight", 180},
            };
        }
        conn->sendDataChannelMessage(bytes::make_binary(to_string(jsonRes)));
    }

    uint32_t GroupCall::addIncomingVideo(const std::string& endpoint, const std::vector<wrtc::SsrcGroup>& ssrcGroup) {
        bool isPresentation = ssrcGroup.size() == 3;
        const auto& conn = isPresentation ? presentationConnection:connection;
        if (!conn) {
            if (!isPresentation) {
                throw ConnectionError("Connection not initialized");
            }
            pendingIncomingPresentations[endpoint] = ssrcGroup;
            return 0;
        }
        if (isPresentation && pendingIncomingPresentations.contains(endpoint)) {
            pendingIncomingPresentations.erase(endpoint);
        }
        const auto ssrc = Safe<wrtc::GroupConnection>(conn)->addIncomingVideo(endpoint, ssrcGroup);
        updateRemoteVideoConstraints(conn);
        endpointsKind.insert({endpoint, isPresentation});
        return ssrc;
    }

    bool GroupCall::removeIncomingVideo(const std::string& endpoint) {
        if (pendingIncomingPresentations.contains(endpoint)) {
            pendingIncomingPresentations.erase(endpoint);
            return true;
        }
        if (!endpointsKind.contains(endpoint)) {
            return false;
        }
        const auto& conn = endpointsKind.at(endpoint) ? presentationConnection : connection;
        if (!conn) {
            throw ConnectionError("Connection not initialized");
        }
        endpointsKind.erase(endpoint);
        return Safe<wrtc::GroupConnection>(conn)->removeIncomingVideo(endpoint);
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

    void GroupCall::setStreamSources(const StreamManager::Mode mode, const MediaDescription& config) const {
        CallInterface::setStreamSources(mode, config);
        if (mode == StreamManager::Mode::Playback && presentationConnection) {
            streamManager->optimizeSources(presentationConnection.get());
        }
    }

    void GroupCall::onUpgrade(const std::function<void(MediaState)>& callback) const {
        streamManager->onUpgrade(callback);
    }

    CallInterface::Type GroupCall::type() const {
        return Type::Group;
    }
} // ntgcalls