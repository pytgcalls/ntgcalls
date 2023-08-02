//
// Created by Laky64 on 02/08/2023.
//

#ifndef NTGCALLS_STREAMSOURCE_H
#define NTGCALLS_STREAMSOURCE_H

#include <iostream>
#include "rtc/common.hpp"
#include "MediaDescription.hpp"

class StreamSource {
protected:

public:
    std::shared_ptr<MediaDescription> desc;
    std::shared_ptr<rtc::MediaChainableHandler> mediaHandler;
    std::shared_ptr<rtc::Track> track;
    std::shared_ptr<rtc::RtcpSrReporter> srReporter;
    bool isVideo;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void loadNextSample() = 0;
    virtual void init() = 0;

    virtual uint64_t getSampleTime_us() = 0;
    virtual uint64_t getSampleDuration_us() = 0;
    virtual rtc::binary getSample() = 0;

    explicit StreamSource(bool isVideo);
};


#endif
