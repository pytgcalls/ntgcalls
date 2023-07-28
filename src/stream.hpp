#include "rtc/rtc.hpp"
#include "MediaDescription.hpp"

class Stream{
private:
    Stream() = default;
    std::shared_ptr<MediaHandler> source;

    static std::shared_ptr<MediaHandler> initAudio();
    static std::shared_ptr<MediaHandler> initVideo();
    void startStreaming() const;

public:

    static Stream Audio();
    static Stream Video();
    std::shared_ptr<rtc::Track> addTrack(const std::shared_ptr<rtc::PeerConnection>& pc) const;
};
