package io.github.pytgcalls;

import io.github.pytgcalls.media.MediaState;

public interface UpgradeCallback {
    void onUpgrade(long chatId, MediaState mediaState);
}
