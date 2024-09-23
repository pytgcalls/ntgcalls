package org.pytgcalls.ntgcalls.p2p;

public class RTCServer {
    public long id;
    public String ipv4;
    public String ipv6;
    public int port;
    public String username;
    public String password;
    public boolean turn;
    public boolean stun;
    public boolean tcp;
    public byte[] peerTag;

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