package org.pytgcalls.ntgcalls.p2p;

public record AuthParams(
    byte[] g_a_or_b,
    long keyFingerprint
) {}
