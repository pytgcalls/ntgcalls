//
// Created by Lauren on 20/11/26.
//

#pragma once

#include <wrtc/interfaces/media/tracks/media_track_interface.hpp>

namespace wrtc::interfaces::media::tracks {
    template<typename Encoding>
    class EncodedTrackInterface final: public MediaTrackInterface {
        utils::synchronized_callback<void(Encoding)> encoding_callback_;

    public:
        EncodedTrackInterface(const std::function<void(bool)>& enable_callback, const std::function<void(Encoding)>& encoding_callback): MediaTrackInterface(enable_callback) {
            encoding_callback_ = encoding_callback;
        }

        ~EncodedTrackInterface() override {
            encoding_callback_ = nullptr;
        }

        void set_encoding(const Encoding& encoding) const {
            (void) encoding_callback_(encoding);
        }
    };
} // wrtc::interfaces::media::tracks
