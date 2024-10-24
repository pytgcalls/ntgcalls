package org.pytgcalls.ntgcalls;

import org.pytgcalls.ntgcalls.media.FrameData;
import org.pytgcalls.ntgcalls.media.StreamDevice;
import org.pytgcalls.ntgcalls.media.StreamMode;

public interface FrameCallback {
    void onFrame(long chatId, long sourceId, StreamMode mode, StreamDevice device, byte[] data, FrameData frameData);
}
