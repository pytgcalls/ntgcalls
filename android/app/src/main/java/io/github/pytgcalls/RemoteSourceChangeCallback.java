package io.github.pytgcalls;

import io.github.pytgcalls.media.RemoteSource;

public interface RemoteSourceChangeCallback {
    void onRemoteSourceChange(long chatId, RemoteSource remoteSource);
}
