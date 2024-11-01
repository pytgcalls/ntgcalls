package io.github.pytgcalls;

public interface ConnectionChangeCallback {
    void onConnectionChange(long chatId, CallNetworkState state);
}
