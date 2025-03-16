package io.github.pytgcalls;

public interface ConnectionChangeCallback {
    void onConnectionChange(long chatId, NetworkInfo state);
}
