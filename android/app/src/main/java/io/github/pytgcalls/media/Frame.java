package io.github.pytgcalls.media;

public class Frame {
    public final long ssrc;
    public final byte[] data;
    public final FrameData frameData;

    Frame(long ssrc, byte[] data, FrameData frameData) {
        this.ssrc = ssrc;
        this.data = data;
        this.frameData = frameData;
    }
}
