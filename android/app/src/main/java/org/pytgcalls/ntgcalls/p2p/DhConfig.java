package org.pytgcalls.ntgcalls.p2p;

public class DhConfig {
    public final int g;
    public final byte[] p;
    public final byte[] random;

    public DhConfig(int g, byte[] p, byte[] random) {
        this.g = g;
        this.p = p;
        this.random = random;
    }
}
