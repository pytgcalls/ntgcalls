package io.github.pytgcalls;

import io.github.pytgcalls.media.FrameData;
import io.github.pytgcalls.media.StreamDevice;
import io.github.pytgcalls.media.StreamMode;

public interface FrameCallback {
    void onFrame(long chatId, long sourceId, StreamMode mode, StreamDevice device, byte[] data, FrameData frameData);
}
