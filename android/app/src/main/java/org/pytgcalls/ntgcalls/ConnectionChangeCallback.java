package org.pytgcalls.ntgcalls;

public interface ConnectionChangeCallback {
    void onConnectionChange(long chatId, ConnectionState state);
}
