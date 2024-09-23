package org.pytgcalls.ntgcalls.p2p;

import java.util.List;

public class Protocol {
    public int minLayer;
    public int maxLayer;
    public boolean udpP2P;
    public boolean udpReflector;
    public List<String> libraryVersions;

    public Protocol(int minLayer, int maxLayer, boolean udpP2P, boolean udpReflector, List<String> libraryVersions) {
        this.minLayer = minLayer;
        this.maxLayer = maxLayer;
        this.udpP2P = udpP2P;
        this.udpReflector = udpReflector;
        this.libraryVersions = libraryVersions;
    }
}
