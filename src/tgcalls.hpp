
#include <iostream>
#include "rtc/rtc.hpp"
#include <nlohmann/json.hpp>
#include "utils.hpp"

using nlohmann::json;

class TgCalls
{
private:
    std::shared_ptr<rtc::PeerConnection> connection;
    std::shared_ptr<rtc::Track> audioTrack;
    std::shared_ptr<rtc::Track> videoTrack;
    
public:
    
    bool start(const std::optional<rtc::Description::Audio>& audio_track,
               const std::optional<rtc::Description::Video>& video_track,
               const std::function<JoinVoiceCallResult(JoinVoiceCallParams)> &joinVoiceCall);
};
