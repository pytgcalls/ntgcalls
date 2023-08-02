//
// Created by Laky64 on 28/07/23.
//

#ifndef NTGCALLS_STREAM_HPP
#define NTGCALLS_STREAM_HPP

#include "rtc/rtc.hpp"
#include "DispatchQueue.hpp"
#include "StreamSource.hpp"
#include <iostream>
#include <shared_mutex>

class Stream{
    enum class State {
        Waiting,
        WaitingForVideo,
        WaitingForAudio,
        Ready
    };

private:
    std::shared_ptr<StreamSource> audioSrc, videoSrc;

    mutable std::mutex mutex;
    std::shared_mutex mutexState;

    State state = State::Waiting;
    mutable bool isRunning = false;
    mutable DispatchQueue dispatchQueue = DispatchQueue("StreamQueue");
    mutable int64_t startTime = 0;

    void setState(State state);

    void preStart(bool isVideo);

    void start() const;

    void sendSample() const;

    void addTrack(const std::shared_ptr<rtc::PeerConnection> &pc,
                  const std::shared_ptr<StreamSource> &mediaSrc);

    std::shared_ptr<StreamSource> unsafePrepareForSample() const;

public:
    Stream(const std::shared_ptr<StreamSource> &audio, const std::shared_ptr<StreamSource> &video);

    ~Stream();

    void addTracks(const std::shared_ptr<rtc::PeerConnection>& pc);

    void stop();

    State getState();

    static void sampleHandler(const std::shared_ptr<StreamSource>& streamSource, const rtc::binary& sample) ;

    void sendInitialNalus();
};

#endif