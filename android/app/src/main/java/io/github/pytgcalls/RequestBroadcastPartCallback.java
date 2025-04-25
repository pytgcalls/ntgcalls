package io.github.pytgcalls;

import io.github.pytgcalls.media.SegmentPartRequest;

public interface RequestBroadcastPartCallback {
    void onRequestBroadcastPart(long chatId, SegmentPartRequest segmentPartRequest);
}
