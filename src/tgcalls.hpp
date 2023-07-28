#include <iostream>
#include "rtc/rtc.hpp"
#include <nlohmann/json.hpp>
#include "SdpBuilder.hpp"

using nlohmann::json;

class TgCalls
{
private:
    std::shared_ptr<rtc::PeerConnection> connection;
    std::shared_ptr<rtc::Track> audioTrack;
    std::shared_ptr<rtc::Track> videoTrack;
    uint32_t audioSource;
    std::vector<uint32_t> sourceGroups;

    std::optional<JoinVoiceCallParams> init(const std::optional<rtc::Description::Audio>& audio_track,
               const std::optional<rtc::Description::Video>& video_track);

public:
    json createCall();
    void setRemoteCallParams(const json& jsonData);
};
