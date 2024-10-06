//
// Created by Laky64 on 03/10/24.
//

#include <wrtc/interfaces/media/rtc_audio_sink.hpp>

namespace wrtc {
    RTCAudioSink::RTCAudioSink(const std::function<void(const Data&)>& callback) {
        dataCallback = callback;
    }

    void RTCAudioSink::OnData(const Data& audio) {
        (void) dataCallback(audio);
    }
} // wrtc