//
// Created by Laky64 on 03/10/24.
//

#pragma once
#include <api/call/audio_sink.h>
#include <wrtc/utils/syncronized_callback.hpp>

namespace wrtc {

    class RTCAudioSink final : public webrtc::AudioSinkInterface {
        synchronized_callback<Data> dataCallback;


    public:
        explicit RTCAudioSink(const std::function<void(const Data&)>& callback);

        void OnData(const Data& audio) override;
    };

} // wrtc
