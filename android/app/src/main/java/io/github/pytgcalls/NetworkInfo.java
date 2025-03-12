package io.github.pytgcalls;

public class NetworkInfo {
    public final Kind kind;
    public final State state;

    public enum Kind {
        NORMAL,
        PRESENTATION,
    }

    public enum State {
        CONNECTING,
        CONNECTED,
        FAILED,
        TIMEOUT,
        CLOSED
    }

    public NetworkInfo(Kind kind, State state) {
        this.kind = kind;
        this.state = state;
    }
}
