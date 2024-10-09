package org.pytgcalls.ntgcalls.media;

public class BaseMediaDescription {
    public final @MediaSource int mediaSource;
    public final String input;

    public BaseMediaDescription(@MediaSource int inputMode, String input) {
        this.mediaSource = inputMode;
        this.input = input;
    }
}
