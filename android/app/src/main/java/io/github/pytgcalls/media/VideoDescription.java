package io.github.pytgcalls.media;

public class VideoDescription extends BaseMediaDescription {
    public final int width, height, fps;

    public VideoDescription(MediaSource mediaSource, String input, boolean keepOpen, int width, int height, int fps) {
        super(mediaSource, input, keepOpen);
        this.width = width;
        this.height = height;
        this.fps = fps;
    }
}
