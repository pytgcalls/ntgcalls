package io.github.pytgcalls.media;

public class BaseMediaDescription {
    public final MediaSource mediaSource;
    public final String input;

    public BaseMediaDescription(MediaSource mediaSource, String input) {
        this.mediaSource = mediaSource;
        this.input = input;
    }
}
