#include "ntgcalls.hpp"

std::optional<JoinVoiceCallParams> NTgCalls::init(const std::optional<Stream> &audioStream,
                    const std::optional<Stream> &videoStream) {
    if (connection != nullptr) {
        throw std::runtime_error("Connection already started");
    }
    std::promise<std::optional<rtc::Description>> descriptionPromise;

    connection = std::make_shared<rtc::PeerConnection>();
    connection -> onGatheringStateChange([this, &descriptionPromise](rtc::PeerConnection::GatheringState state) {
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            descriptionPromise.set_value(connection->localDescription());
        }
    });

    if (audioStream.has_value()) {
        audioTrack = audioStream->addTrack(connection);

    }

    if (videoStream.has_value()) {
        audioTrack = videoStream->addTrack(connection);
    }
    connection->setLocalDescription();

    std::future<std::optional<rtc::Description>> descriptionFuture = descriptionPromise.get_future();
    const std::optional<rtc::Description> localDescription = descriptionFuture.get();
    if (!localDescription.has_value()) {
        throw std::runtime_error("LocalDescription not found");
    }

    const auto sdp = parseSdp(localDescription.value());
    if (!sdp.ufrag || !sdp.pwd || !sdp.hash || !sdp.fingerprint) {
        return std::nullopt;
    }

    std::cout << std::string(localDescription.value());
    audioSource = sdp.audioSource;
    sourceGroups = sdp.source_groups;
    return JoinVoiceCallParams {
            sdp.ufrag.value(),
            sdp.pwd.value(),
            sdp.hash.value(),
            "active",
            sdp.fingerprint.value()
    };
}

json NTgCalls::createCall() {
    auto audio = Stream::Audio();
    auto video = Stream::Video();
    auto sdp = init(audio, video);

    if (!sdp.has_value()) {
        throw std::runtime_error("Sdp has no value");
    }

    auto params = sdp.value();
    json jsonRes = {
            {"ufrag", params.ufrag},
            {"pwd", params.pwd},
            {"fingerprints", {{
                {"hash", params.hash},
                {"setup", params.setup},
                {"fingerprint", params.fingerprint}
            }}},
            {"ssrc", audioSource},
    };

    if (!sourceGroups.empty()){
        jsonRes["ssrc-groups"] = {
                {"semantics", "FID"},
                {"sources", sourceGroups}
        };
    }
    return jsonRes;

}

void NTgCalls::setRemoteCallParams(const json& jsonData) {
    if (!jsonData["rtmp"].is_null()) {
        throw std::runtime_error("Needed rtmp connection");
    }
    if (jsonData["transport"].is_null()) {
        throw std::runtime_error("Transport not found");
    }
    Conference conference;
    try {
        conference = {
                getMilliseconds(),
                {
                        jsonData["transport"]["ufrag"].get<std::string>(),
                        jsonData["transport"]["pwd"].get<std::string>()
                },
                {
                        {
                            audioSource,
                            sourceGroups,
                        }
                }
        };
        for (const auto& item : jsonData["transport"]["fingerprints"].items()) {
            conference.transport.fingerprints.push_back({
                item.value()["hash"],
                item.value()["fingerprint"],
            });
        }
        for (const auto& item : jsonData["transport"]["candidates"].items()) {
            conference.transport.candidates.push_back({
                item.value()["generation"].get<std::string>(),
                item.value()["component"].get<std::string>(),
                item.value()["protocol"].get<std::string>(),
                item.value()["port"].get<std::string>(),
                item.value()["ip"].get<std::string>(),
                item.value()["foundation"].get<std::string>(),
                item.value()["id"].get<std::string>(),
                item.value()["priority"].get<std::string>(),
                item.value()["type"].get<std::string>(),
                item.value()["network"].get<std::int64_t>()
            });
        }
    } catch (...) {
        throw std::runtime_error("Invalid transport");
    }
    rtc::Description answer(SdpBuilder::fromConference(conference), "answer");
    connection->setRemoteDescription(answer);
}