package io.github.pytgcalls.p2p;

public class SubchainRequest {
    public final int subchain;
    public final int height;
    public final int limit;

    public SubchainRequest(int subchain, int height, int limit) {
        this.subchain = subchain;
        this.height = height;
        this.limit = limit;
    }
}