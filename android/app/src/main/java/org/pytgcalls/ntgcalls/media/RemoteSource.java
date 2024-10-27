package org.pytgcalls.ntgcalls.media;

public class RemoteSource {
    public final int ssrc;
    public final SourceState state;
    public final StreamDevice device;

    public RemoteSource(int ssrc, SourceState state, StreamDevice device) {
        this.ssrc = ssrc;
        this.state = state;
        this.device = device;
    }
}
