package io.github.pytgcalls.media;

public class BaseMediaDescription {
    public final MediaSource mediaSource;
    public final String input;
    public final boolean keepOpen;
    public BaseMediaDescription(MediaSource mediaSource, String input, boolean keepOpen) {
        this.mediaSource = mediaSource;
        this.input = input;
        this.keepOpen = keepOpen;
    }
}
