package org.pytgcalls.ntgcalls.media;

public class MediaDescription {
    public AudioDescription audio;
    public VideoDescription video;

    public MediaDescription(AudioDescription audio, VideoDescription video) {
        this.audio = audio;
        this.video = video;
    }

    public MediaDescription(AudioDescription audio) {
        this.audio = audio;
    }

    public MediaDescription(VideoDescription video) {
        this.video = video;
    }
}
