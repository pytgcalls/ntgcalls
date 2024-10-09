package org.pytgcalls.ntgcalls;

public class CallNetworkState {
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

    public CallNetworkState(Kind kind, State state) {
        this.kind = kind;
        this.state = state;
    }
}
