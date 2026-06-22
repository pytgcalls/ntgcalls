package io.github.pytgcalls;

public interface OutboundBlockCallback {
    void onOutboundBlock(long chatId, byte[] block);
}