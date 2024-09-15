package org.pytgcalls.ntgcalls.media;

public class BaseMediaDescription {
    @InputMode int inputMode;
    String input;

    public BaseMediaDescription(@InputMode int inputMode, String input) {
        this.inputMode = inputMode;
        this.input = input;
    }
}
