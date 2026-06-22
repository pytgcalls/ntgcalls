package io.github.pytgcalls.media;

public class MediaState {
    public final boolean muted, videoPaused, videoStopped, presentationPaused, presentationStopped;

    public MediaState(boolean muted, boolean videoPaused, boolean videoStopped, boolean presentationPaused, boolean presentationStopped) {
        this.muted = muted;
        this.videoPaused = videoPaused;
        this.videoStopped = videoStopped;
        this.presentationPaused = presentationPaused;
        this.presentationStopped = presentationStopped;
    }
}