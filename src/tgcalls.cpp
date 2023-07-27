#include "tgcalls.hpp"

bool TgCalls::start(const std::optional<rtc::Description::Audio> &audio_track,
                    const std::optional<rtc::Description::Video> &video_track,
                    const std::function<JoinVoiceCallResult(JoinVoiceCallParams)> &joinVoiceCall) {
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

    if (audio_track.has_value()) {
        audioTrack = connection->addTrack(audio_track.value());
    }

    if (video_track.has_value()) {
        videoTrack = connection->addTrack(video_track.value());
    }
    connection->setLocalDescription();

    std::future<std::optional<rtc::Description>> descriptionFuture = descriptionPromise.get_future();
    const std::optional<rtc::Description> localDescription = descriptionFuture.get();
    if (!localDescription.has_value()) {
        throw std::runtime_error("LocalDescription not found");
    }

    const auto sdp = parseSdp(localDescription.value());
    if (!sdp.ufrag || !sdp.pwd || !sdp.hash || !sdp.fingerprint) {
        return false;
    }

    std::cout << std::string(localDescription.value());

    const auto result = joinVoiceCall({
        sdp.ufrag.value(),
        sdp.pwd.value(),
        sdp.hash.value(),
        "active",
        sdp.fingerprint.value(),
        sdp.audioSource,
        sdp.source_groups
    });
    return true;
}