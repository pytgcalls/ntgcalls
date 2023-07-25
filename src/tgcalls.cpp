#include "tgcalls.hpp"

TgCalls::TgCalls(const std::int64_t chat_id) {
        chatId = chat_id;
}

void TgCalls::start(const rtc::Description::Audio audio_track, const rtc::Description::Video video_track) {
    if (connection != nullptr) {
        throw std::runtime_error("Connection already started");
    }
    bool alreadySolved = false;
    bool resultSolve = false;

    connection = std::make_shared<rtc::PeerConnection>();
    connection -> onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            
        }
    });
}
