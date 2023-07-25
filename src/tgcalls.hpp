
#include <iostream>
#include "rtc/rtc.hpp"
#include <nlohmann/json.hpp>

class TgCalls
{
private:
    /* data */
    std::shared_ptr<rtc::PeerConnection> connection;
    std::shared_ptr<rtc::Track> audioTrack;
    std::shared_ptr<rtc::Track> videoTrack;
    std::int64_t chatId;
    
public:
    TgCalls(const std::int64_t chat_id);
    
    void start(const rtc::Description::Audio audio_track, const rtc::Description::Video video_track);

    void start(const rtc::Description::Audio audio_track);
};
