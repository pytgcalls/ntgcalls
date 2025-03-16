package io.github.pytgcalls;

public interface SignalingDataCallback {
    void onSignalingData(long chatId, byte[] data);
}
