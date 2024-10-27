package org.pytgcalls.ntgcalls;

import org.pytgcalls.ntgcalls.media.RemoteSource;

public interface RemoteSourceChangeCallback {
    void onRemoteSourceChange(long chatId, RemoteSource remoteSource);
}
