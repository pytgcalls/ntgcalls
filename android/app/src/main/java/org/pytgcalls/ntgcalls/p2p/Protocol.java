package org.pytgcalls.ntgcalls.p2p;

import java.util.List;

public record Protocol(int minLayer, int maxLayer, boolean udpP2P, boolean udpReflector, List<String> libraryVersions) {}
