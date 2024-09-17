package org.pytgcalls.ntgcalls;

import org.pytgcalls.ntgcalls.media.MediaState;

public interface UpgradeCallback {
    void onUpgrade(long chatId, MediaState mediaState);
}
