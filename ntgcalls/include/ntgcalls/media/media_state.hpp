//
// Created by Lauren on 29/08/23.
//

#pragma once

namespace ntgcalls::media {

    struct MediaState {
        bool muted;
        bool video_paused;
        bool video_stopped;
        bool presentation_paused;
        bool presentation_stopped;
    };

} // ntgcalls::media
