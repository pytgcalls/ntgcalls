package org.pytgcalls.ntgcalls.p2p;

public class DhConfig {
    public int g;
    public byte[] p;
    public byte[] random;

    public DhConfig(int g, byte[] p, byte[] random) {
        this.g = g;
        this.p = p;
        this.random = random;
    }
}
