package io.github.pytgcalls.media;

public class RemoteSource {
    public final int ssrc;
    public final StreamStatus state;
    public final StreamDevice device;

    public RemoteSource(int ssrc, StreamStatus state, StreamDevice device) {
        this.ssrc = ssrc;
        this.state = state;
        this.device = device;
    }
}
