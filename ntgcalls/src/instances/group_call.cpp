//
// Created by Laky64 on 15/03/2024.
//

#include <ntgcalls/instances/group_call.hpp>

#include <future>

#include <ntgcalls/exceptions.hpp>
#include <wrtc/interfaces/group_connection.hpp>
#include <wrtc/models/response_payload.hpp>

namespace ntgcalls {

    void GroupCall::stop() {
        stopPresentation();
        CallInterface::stop();
    }

    std::string GroupCall::init() {
        RTC_LOG(LS_INFO) << "Initializing group call";
        if (connection) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }
        connection = std::make_shared<wrtc::GroupConnection>(false);
        connection->open();
        RTC_LOG(LS_INFO) << "Group call initialized";
        streamManager->setStreamSources(StreamManager::Mode::Capture);
        streamManager->setStreamSources(StreamManager::Mode::Playback);
        streamManager->optimizeSources(connection.get());

        std::weak_ptr weak(shared_from_this());
        connection->onDataChannelOpened([weak] {
            const auto strong = std::static_pointer_cast<GroupCall>(weak.lock());
            if (!strong) {
                return;
            }
            RTC_LOG(LS_VERBOSE) << "Data channel opened";
            updateRemoteVideoConstraints(Safe<wrtc::GroupConnection>(strong->connection));
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
        if (getConnectionMode() != wrtc::ConnectionMode::Rtc) {
            RTC_LOG(LS_ERROR) << "Presentation connection requires RTC connection";
            throw RTCConnectionNeeded("Presentation connection requires RTC connection");
        }
        RTC_LOG(LS_INFO) << "Initializing screen sharing";
        if (presentationConnection) {
            RTC_LOG(LS_ERROR) << "Screen sharing already initialized";
            throw ConnectionError("Screen sharing already initialized");
        }
        presentationConnection = std::make_shared<wrtc::GroupConnection>(true);
        presentationConnection->open();
        streamManager->optimizeSources(presentationConnection.get());
        std::weak_ptr weak(shared_from_this());
        presentationConnection->onDataChannelOpened([weak] {
            const auto strong = std::static_pointer_cast<GroupCall>(weak.lock());
            if (!strong) {
                return;
            }
            RTC_LOG(LS_VERBOSE) << "Data channel opened";
            updateRemoteVideoConstraints(Safe<wrtc::GroupConnection>(strong->presentationConnection));
        });
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Speaker, presentationConnection.get());
        streamManager->addTrack(StreamManager::Mode::Capture, StreamManager::Device::Screen, presentationConnection.get());
        RTC_LOG(LS_INFO) << "Screen sharing initialized";
        return presentationConnection->getJoinPayload();
    }

    void GroupCall::connect(const std::string& jsonData, const bool isPresentation) {
        RTC_LOG(LS_VERBOSE) << "Connecting to group call";
        const auto &conn = isPresentation ? presentationConnection : connection;
        if (!conn) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionError("Connection not initialized");
        }

        wrtc::ResponsePayload payload(jsonData);
        wrtc::ConnectionMode connectionMode;
        if (payload.isRtmp) {
            connectionMode = wrtc::ConnectionMode::Rtmp;
        } else if (payload.isStream) {
            connectionMode = wrtc::ConnectionMode::Stream;
        } else {
            connectionMode = wrtc::ConnectionMode::Rtc;
        }

        const auto currentConnectionMode = conn->getConnectionMode();
        if (currentConnectionMode == connectionMode || currentConnectionMode == wrtc::ConnectionMode::Rtmp) {
            RTC_LOG(LS_ERROR) << "Connection already made";
            throw ConnectionError("Connection already made");
        }

        if (currentConnectionMode == wrtc::ConnectionMode::Rtc && connectionMode != wrtc::ConnectionMode::Stream) {
            RTC_LOG(LS_ERROR) << "Cannot switch connection mode from RTC to MTProto";
            throw ConnectionError("Cannot switch connection mode from RTC to MTProto");
        }

        if (connectionMode == wrtc::ConnectionMode::Rtmp && streamManager->hasReaders()) {
            RTC_LOG(LS_ERROR) << "Streaming is not supported when using RTMP";
            throw RTMPStreamingUnsupported("Streaming is not supported when using RTMP");
        }

        Safe<wrtc::GroupConnection>(conn)->setConnectionMode(connectionMode);
        if (connectionMode == wrtc::ConnectionMode::Rtc) {
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
            streamManager->optimizeSources(conn.get());
            Safe<wrtc::GroupConnection>(conn)->createChannels(payload.media);
            RTC_LOG(LS_VERBOSE) << "Remote parameters set";
        } else {
            std::weak_ptr weak(shared_from_this());
            Safe<wrtc::GroupConnection>(conn)->onRequestBroadcastPart([weak](const wrtc::SegmentPartRequest& request){
                const auto strong = std::static_pointer_cast<GroupCall>(weak.lock());
                if (!strong) {
                    return;
                }
                (void) strong->segmentPartRequestCallback(request);
            });
            Safe<wrtc::GroupConnection>(conn)->onRequestBroadcastTimestamp([weak]{
                const auto strong = std::static_pointer_cast<GroupCall>(weak.lock());
                if (!strong) {
                    return;
                }
                (void) strong->broadcastTimestampCallback();
            });
            Safe<wrtc::GroupConnection>(conn)->connectMediaStream();
            streamManager->optimizeSources(conn.get());
            RTC_LOG(LS_VERBOSE) << "MTProto stream attached";
        }
        setConnectionObserver(
            conn,
            isPresentation ? NetworkInfo::Kind::Presentation : NetworkInfo::Kind::Normal
        );
    }

    void GroupCall::updateRemoteVideoConstraints(const wrtc::GroupConnection* conn) {
        json jsonRes = {
            {"colibriClass", "ReceiverVideoConstraints"},
            {"constraints", json::object()},
            {"defaultConstraints", {{"maxHeight", 0}}},
            {"onStageEndpoints", json::array()}
        };
        for (const auto& endpoint : conn->getEndpoints()) {
            jsonRes["constraints"][endpoint] = {
                {"maxHeight", 360},
                {"minHeight", 180},
            };
        }
        conn->sendDataChannelMessage(bytes::make_binary(to_string(jsonRes)));
    }

    uint32_t GroupCall::addIncomingVideo(const std::string& endpoint, const std::vector<wrtc::SsrcGroup>& ssrcGroup) const {
        const auto& conn = Safe<wrtc::GroupConnection>(connection);
        if (!conn) {
            throw ConnectionError("Connection not initialized");
        }
        const auto ssrc = conn->addIncomingVideo(endpoint, ssrcGroup);
        if (getConnectionMode() == wrtc::ConnectionMode::Rtc) updateRemoteVideoConstraints(conn);
        return ssrc;
    }

    bool GroupCall::removeIncomingVideo(const std::string& endpoint) const {
        const auto& conn = Safe<wrtc::GroupConnection>(connection);
        if (!conn) {
            throw ConnectionError("Connection not initialized");
        }
        return conn->removeIncomingVideo(endpoint);
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
        if (mode == StreamManager::Mode::Capture && getConnectionMode() == wrtc::ConnectionMode::Rtmp) {
            RTC_LOG(LS_ERROR) << "Streaming is not supported when using RTMP";
            throw RTMPStreamingUnsupported("Streaming is not supported when using RTMP");
        }
        CallInterface::setStreamSources(mode, config);
        if (mode == StreamManager::Mode::Playback && presentationConnection) {
            streamManager->optimizeSources(presentationConnection.get());
        }
    }

    void GroupCall::onUpgrade(const std::function<void(MediaState)>& callback) const {
        streamManager->onUpgrade(callback);
    }

    void GroupCall::sendBroadcastPart(const int64_t segmentID, const int32_t partID, const wrtc::MediaSegment::Part::Status status, const bool qualityUpdate, const std::optional<bytes::binary>& data) const {
        const auto groupConnection = Safe<wrtc::GroupConnection>(connection);
        if (!groupConnection) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionError("Connection not initialized");
        }
        groupConnection->sendBroadcastPart(segmentID, partID, status, qualityUpdate, data);
    }

    void GroupCall::onRequestedBroadcastPart(const std::function<void(wrtc::SegmentPartRequest)>& callback) {
        segmentPartRequestCallback = callback;
    }

    void GroupCall::sendBroadcastTimestamp(const int64_t timestamp) const {
        const auto groupConnection = Safe<wrtc::GroupConnection>(connection);
        if (!groupConnection) {
            RTC_LOG(LS_ERROR) << "Connection not initialized";
            throw ConnectionError("Connection not initialized");
        }
        groupConnection->sendBroadcastTimestamp(timestamp);
    }

    void GroupCall::onRequestedBroadcastTimestamp(const std::function<void()>& callback) {
        broadcastTimestampCallback = callback;
    }

    CallInterface::Type GroupCall::type() const {
        return Type::Group;
    }
} // ntgcalls