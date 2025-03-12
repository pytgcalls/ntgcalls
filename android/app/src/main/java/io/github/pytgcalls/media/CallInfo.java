package io.github.pytgcalls.media;

public class CallInfo {
    public final StreamStatus playback, capture;

    public CallInfo(StreamStatus playback, StreamStatus capture) {
        this.playback = playback;
        this.capture = capture;
    }
}
