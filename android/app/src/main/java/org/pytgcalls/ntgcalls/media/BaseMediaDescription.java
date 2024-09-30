package org.pytgcalls.ntgcalls.media;

public class BaseMediaDescription {
    public final @InputMode int inputMode;
    public final String input;

    public BaseMediaDescription(@InputMode int inputMode, String input) {
        this.inputMode = inputMode;
        this.input = input;
    }
}
