#include "ntgcalls.hpp"
#include "io/SamplesReader.hpp"

std::optional<JoinVoiceCallParams> NTgCalls::init(const std::shared_ptr<Stream> &mediaStream) {
    if (connection != nullptr) {
        throw std::runtime_error("Connection already started");
    }
    rtc::InitLogger(rtc::LogLevel::Debug);
    std::promise<std::optional<rtc::Description>> descriptionPromise;

    // https://github.com/morethanwords/tweb/blob/master/src/lib/calls/groupCallConnectionInstance.ts#L52-L62
    rtc::Configuration config;
    config.iceTransportPolicy = rtc::TransportPolicy::All;
    config.enableIceUdpMux = true;
    config.disableAutoNegotiation = true;

    connection = std::make_shared<rtc::PeerConnection>(config);
    connection -> onGatheringStateChange([this, &descriptionPromise](rtc::PeerConnection::GatheringState state) {
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            descriptionPromise.set_value(connection->localDescription());
        }
    });

    mediaStream->addTracks(connection);

    connection->setLocalDescription(rtc::Description::Type::Offer);

    std::future<std::optional<rtc::Description>> descriptionFuture = descriptionPromise.get_future();
    const std::optional<rtc::Description> localDescription = descriptionFuture.get();
    if (!localDescription.has_value()) {
        throw std::runtime_error("LocalDescription not found");
    }
    /* TESTING ON browser.html
    json message = {{"type", localDescription->typeString()},
                    {"sdp", std::string(localDescription.value())}};
    std::cout << message << std::endl;*/

    const auto sdp = SdpBuilder::parseSdp(localDescription.value());
    if (!sdp.ufrag || !sdp.pwd || !sdp.hash || !sdp.fingerprint) {
        return std::nullopt;
    }

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

std::string NTgCalls::createCall(const std::string& audioPath) {
    stream = std::make_shared<Stream>(std::make_shared<SamplesReader>(audioPath, ".opus"));
    auto sdp = init(stream);

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
    return to_string(jsonRes);
}

void NTgCalls::setRemoteCallParams(const std::string& jsonData) {
    auto data = json::parse(jsonData);
    if (!data["rtmp"].is_null()) {
        throw std::runtime_error("Needed rtmp connection");
    }
    if (data["transport"].is_null()) {
        throw std::runtime_error("Transport not found");
        /* TESTING ON browser.html
        std::cout << "RTP video stream expected on localhost:6000" << std::endl;
        std::cout << "Please copy/paste the answer provided by the browser: " << std::endl;
        std::string sdp;
        std::getline(std::cin, sdp);

        json j = json::parse(sdp);
        rtc::Description answer(j["sdp"].get<std::string>(), j["type"].get<std::string>());
        connection->setRemoteDescription(answer);
        return;*/
    }
    data = data["transport"];
    Conference conference;
    try {
        conference = {
                getMilliseconds(),
                {
                        data["ufrag"].get<std::string>(),
                        data["pwd"].get<std::string>()
                },
                {
                        {
                            audioSource,
                            sourceGroups,
                        }
                }
        };
        for (const auto& item : data["fingerprints"].items()) {
            conference.transport.fingerprints.push_back({
                item.value()["hash"],
                item.value()["fingerprint"],
            });
        }
        for (const auto& item : data["candidates"].items()) {
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
                item.value()["network"].get<std::string>()
            });
        }
    } catch (...) {
        throw std::runtime_error("Invalid transport");
    }
    std::promise<bool> waitConnection;
    connection->onStateChange([&waitConnection](rtc::PeerConnection::State state) {
        std::cout << "[STATE: " << state << "]";
        if (state == rtc::PeerConnection::State::Failed || state == rtc::PeerConnection::State::Closed) {
            waitConnection.set_value(false);
        } else if (state == rtc::PeerConnection::State::Connected) {
            waitConnection.set_value(true);
        }
    });
    rtc::Description answer(SdpBuilder::fromConference(conference), rtc::Description::Type::Answer);
    connection->setRemoteDescription(answer);
    if (!waitConnection.get_future().get()) {
        throw std::runtime_error("WebRTC connection failed");
    }
}