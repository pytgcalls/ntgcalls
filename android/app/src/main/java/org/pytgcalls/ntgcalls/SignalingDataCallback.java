package org.pytgcalls.ntgcalls;

public interface SignalingDataCallback {
    void onSignalingData(long chatId, byte[] data);
}
