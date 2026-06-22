package io.github.pytgcalls;

import io.github.pytgcalls.p2p.SubchainRequest;

public interface SubchainRequestCallback {
    void onSubchainRequest(long chatId, SubchainRequest subchainRequest);
}