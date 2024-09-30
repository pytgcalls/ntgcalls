package org.pytgcalls.ntgcalls.media;

public class MediaStatus {
    public final StreamStatus playback, capture;

    public MediaStatus(StreamStatus playback, StreamStatus capture) {
        this.playback = playback;
        this.capture = capture;
    }
}
