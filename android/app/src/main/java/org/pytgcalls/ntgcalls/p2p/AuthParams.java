package org.pytgcalls.ntgcalls.p2p;

public class AuthParams {
    public byte[] g_a_or_b;
    public long keyFingerprint;

    public AuthParams(byte[] g_a_or_b, long keyFingerprint) {
        this.g_a_or_b = g_a_or_b;
        this.keyFingerprint = keyFingerprint;
    }
}