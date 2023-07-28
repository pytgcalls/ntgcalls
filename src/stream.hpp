#include "rtc/rtc.hpp"
#include "utils.hpp"

class Stream{
private:
    rtc::Description::Audio audioSource;

    rtc::Description::Audio initAudio();
    void addAudio();
public:

    Stream();
};
