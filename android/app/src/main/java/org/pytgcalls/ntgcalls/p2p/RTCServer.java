package org.pytgcalls.ntgcalls.p2p;

public class RTCServer {
    public final long id;
    public final String ipv4;
    public final String ipv6;
    public final int port;
    public final String username;
    public final String password;
    public final boolean turn;
    public final boolean stun;
    public final boolean tcp;
    public final byte[] peerTag;

    public RTCServer(long id, String ipv4, String ipv6, int port, String username, String password, boolean turn, boolean stun, boolean tcp, byte[] peerTag) {
        this.id = id;
        this.ipv4 = ipv4;
        this.ipv6 = ipv6;
        this.port = port;
        this.username = username;
        this.password = password;
        this.turn = turn;
        this.stun = stun;
        this.tcp = tcp;
        this.peerTag = peerTag;
    }
}