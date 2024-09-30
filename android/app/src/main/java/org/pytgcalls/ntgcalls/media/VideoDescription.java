package org.pytgcalls.ntgcalls.media;

public class VideoDescription extends BaseMediaDescription {
    public final int width, height, fps;

    public VideoDescription(@InputMode int flags, String input, int width, int height, int fps) {
        super(flags, input);
        this.width = width;
        this.height = height;
        this.fps = fps;
    }
}
