package org.pytgcalls.ntgcalls.media;

public class MediaState {
    public final boolean muted, videoPaused, videoStopped;

    public MediaState(boolean muted, boolean videoPaused, boolean videoStopped) {
        this.muted = muted;
        this.videoPaused = videoPaused;
        this.videoStopped = videoStopped;
    }
}
