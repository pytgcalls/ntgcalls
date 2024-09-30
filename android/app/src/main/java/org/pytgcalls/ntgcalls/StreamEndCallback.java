package org.pytgcalls.ntgcalls;

import org.pytgcalls.ntgcalls.media.StreamDevice;
import org.pytgcalls.ntgcalls.media.StreamType;

public interface StreamEndCallback {
    void onStreamEnd(long chatId, StreamType type, StreamDevice device);
}
