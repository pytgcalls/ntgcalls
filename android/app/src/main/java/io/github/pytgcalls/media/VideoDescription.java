package io.github.pytgcalls.media;

public class VideoDescription extends BaseMediaDescription {
    public final int width, height, fps;

    public VideoDescription(MediaSource mediaSource, String input, int width, int height, int fps) {
        super(mediaSource, input);
        this.width = width;
        this.height = height;
        this.fps = fps;
    }
}
