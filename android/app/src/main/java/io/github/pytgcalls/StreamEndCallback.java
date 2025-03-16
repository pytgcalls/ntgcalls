package io.github.pytgcalls;

import io.github.pytgcalls.media.StreamDevice;
import io.github.pytgcalls.media.StreamType;

public interface StreamEndCallback {
    void onStreamEnd(long chatId, StreamType type, StreamDevice device);
}
