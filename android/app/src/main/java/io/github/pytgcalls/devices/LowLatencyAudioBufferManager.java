package io.github.pytgcalls.devices;

import android.media.AudioTrack;
import android.os.Build;

import org.webrtc.Logging;

public class LowLatencyAudioBufferManager {
    private static final String TAG = "LowLatencyAudioBufferManager";
    private int prevUnderrunCount = 0;
    private int ticksUntilNextDecrease = 10;
    private boolean keepLoweringBufferSize = true;
    private int bufferIncreaseCounter = 0;

    public void maybeAdjustBufferSize(AudioTrack audioTrack) {
        if (Build.VERSION.SDK_INT >= 26) {
            int underrunCount = audioTrack.getUnderrunCount();
            int bufferSize10ms;
            int currentBufferSize;
            if (underrunCount > this.prevUnderrunCount) {
                if (this.bufferIncreaseCounter < 5) {
                    bufferSize10ms = audioTrack.getBufferSizeInFrames();
                    currentBufferSize = bufferSize10ms + audioTrack.getPlaybackRate() / 100;
                    Logging.d("LowLatencyAudioBufferManager", "Underrun detected! Increasing AudioTrack buffer size from " + bufferSize10ms + " to " + currentBufferSize);
                    audioTrack.setBufferSizeInFrames(currentBufferSize);
                    ++this.bufferIncreaseCounter;
                }

                this.keepLoweringBufferSize = false;
                this.prevUnderrunCount = underrunCount;
                this.ticksUntilNextDecrease = 10;
            } else if (this.keepLoweringBufferSize) {
                --this.ticksUntilNextDecrease;
                if (this.ticksUntilNextDecrease <= 0) {
                    bufferSize10ms = audioTrack.getPlaybackRate() / 100;
                    currentBufferSize = audioTrack.getBufferSizeInFrames();
                    int newBufferSize = Math.max(bufferSize10ms, currentBufferSize - bufferSize10ms);
                    if (newBufferSize != currentBufferSize) {
                        Logging.d("LowLatencyAudioBufferManager", "Lowering AudioTrack buffer size from " + currentBufferSize + " to " + newBufferSize);
                        audioTrack.setBufferSizeInFrames(newBufferSize);
                    }

                    this.ticksUntilNextDecrease = 10;
                }
            }
        }
    }
}
