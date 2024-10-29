package org.pytgcalls.ntgcalls.media;

public class FrameData {
    public final long absoluteCaptureTimestampMs;
    public final int width, height, rotation;

    public FrameData(long absoluteCaptureTimestampMs, int width, int height, int rotation) {
        this.absoluteCaptureTimestampMs = absoluteCaptureTimestampMs;
        this.width = width;
        this.height = height;
        this.rotation = rotation;
    }
}
