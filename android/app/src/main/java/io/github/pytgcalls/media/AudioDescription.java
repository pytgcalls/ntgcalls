package io.github.pytgcalls.media;

public class AudioDescription extends BaseMediaDescription {
    public final int sampleRate, bitsPerSample, channelCount;

    public AudioDescription(@MediaSource int mediaSource, String input, int sampleRate, int bitsPerSample, int channelCount) {
        super(mediaSource, input);
        this.sampleRate = sampleRate;
        this.channelCount = channelCount;
        this.bitsPerSample = bitsPerSample;
    }
}