package io.github.pytgcalls;

import java.util.List;

import io.github.pytgcalls.media.Frame;
import io.github.pytgcalls.media.FrameData;
import io.github.pytgcalls.media.StreamDevice;
import io.github.pytgcalls.media.StreamMode;

public interface FrameCallback {
    void onFrames(long chatId, StreamMode mode, StreamDevice device, List<Frame> frames);
}
