package io.github.pytgcalls.p2p;

public class ConferenceJoinParams {
    public final String payload;
    public final byte[] publicKey;
    public final byte[] block;

    public ConferenceJoinParams(String payload, byte[] publicKey, byte[] block) {
        this.payload = payload;
        this.publicKey = publicKey;
        this.block = block;
    }
}