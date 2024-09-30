package org.pytgcalls.ntgcalls.media;

public class AudioDescription extends BaseMediaDescription {
    public final int sampleRate, bitsPerSample, channelCount;

    public AudioDescription(@InputMode int flags, String input, int sampleRate, int bitsPerSample, int channelCount) {
        super(flags, input);
        this.sampleRate = sampleRate;
        this.channelCount = channelCount;
        this.bitsPerSample = bitsPerSample;
    }
}