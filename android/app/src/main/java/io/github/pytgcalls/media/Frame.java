package io.github.pytgcalls.media;

public class Frame {
    long ssrc;
    byte[] data;
    FrameData frameData;

    Frame(long ssrc, byte[] data, FrameData frameData) {
        this.ssrc = ssrc;
        this.data = data;
        this.frameData = frameData;
    }
}
