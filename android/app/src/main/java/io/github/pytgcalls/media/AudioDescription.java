package io.github.pytgcalls.media;

public class AudioDescription extends BaseMediaDescription {
    public final int sampleRate, channelCount;

    public AudioDescription(MediaSource mediaSource, String input, int sampleRate, int channelCount) {
        super(mediaSource, input);
        this.sampleRate = sampleRate;
        this.channelCount = channelCount;
    }
}