package org.pytgcalls.ntgcalls.p2p;

public record DhConfig(
    int g,
    byte[] p,
    byte[] random
) {}
