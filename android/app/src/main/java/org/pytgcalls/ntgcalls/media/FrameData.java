package org.pytgcalls.ntgcalls.media;

public class FrameData {
    public final long absoluteCaptureTimestampMs;
    public final int rotation;

    public FrameData(long absoluteCaptureTimestampMs, int rotation) {
        this.absoluteCaptureTimestampMs = absoluteCaptureTimestampMs;
        this.rotation = rotation;
    }
}
