package io.github.pytgcalls.media;

public class SsrcMapping {
    public final long userId;
    public final int ssrc;

    public SsrcMapping(long userId, int ssrc) {
        this.userId = userId;
        this.ssrc = ssrc;
    }
}