package org.pytgcalls.ntgcalls.media;

public class BaseMediaDescription {
    public final @MediaSource int mediaSource;
    public final String input;

    public BaseMediaDescription(@MediaSource int mediaSource, String input) {
        this.mediaSource = mediaSource;
        this.input = input;
    }
}
