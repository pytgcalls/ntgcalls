package org.pytgcalls.ntgcalls.p2p;

public record RTCServer(
    long id,
    String ipv4,
    String ipv6,
    int port,
    String username,
    String password,
    boolean turn,
    boolean stun,
    boolean tcp,
    byte[] peerTag
) {}
