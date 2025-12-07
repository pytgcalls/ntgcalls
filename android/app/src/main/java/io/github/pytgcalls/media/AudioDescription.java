package io.github.pytgcalls.media;

public class AudioDescription extends BaseMediaDescription {
    public final int sampleRate, channelCount;

    public AudioDescription(MediaSource mediaSource, String input, boolean keepOpen, int sampleRate, int channelCount) {
        super(mediaSource, input, keepOpen);
        this.sampleRate = sampleRate;
        this.channelCount = channelCount;
    }
}