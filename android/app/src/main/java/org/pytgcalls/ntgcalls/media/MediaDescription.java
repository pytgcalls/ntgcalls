package org.pytgcalls.ntgcalls.media;

public class MediaDescription {
    public final AudioDescription microphone, speaker;
    public final VideoDescription camera, screen;

    public MediaDescription(AudioDescription microphone, AudioDescription speaker, VideoDescription camera, VideoDescription screen) {
        this.microphone = microphone;
        this.speaker = speaker;
        this.camera = camera;
        this.screen = screen;
    }
}
