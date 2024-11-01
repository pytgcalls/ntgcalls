package io.github.pytgcalls.devices;

import android.annotation.TargetApi;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Build;
import android.os.Process;

import androidx.annotation.RequiresApi;
import androidx.annotation.RequiresPermission;

import java.nio.ByteBuffer;

@SuppressWarnings("unused")
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
class JavaAudioDeviceModule {
    protected ByteBuffer byteBuffer;
    protected static final int CALLBACK_BUFFER_SIZE_MS = 10;
    protected static final int BUFFERS_PER_SECOND = 1000 / CALLBACK_BUFFER_SIZE_MS;
    protected static final int BUFFER_SIZE_FACTOR = 2;
    protected static final int DEFAULT_AUDIO_FORMAT = AudioFormat.ENCODING_PCM_16BIT;

    private AudioRecord audioRecord;
    private AudioTrack audioTrack;
    private AudioEffects effects = new AudioEffects();
    private AudioThread audioThread;
    private final boolean isCapture;

    @SuppressWarnings({"unused", "FieldCanBeLocal"})
    private final long nativePointer;

    @RequiresPermission(android.Manifest.permission.RECORD_AUDIO)
    public JavaAudioDeviceModule(boolean isCapture, int sampleRate, int channels, long nativePointer) {
        final int bytesPerFrame = channels * DEFAULT_AUDIO_FORMAT;
        final int framesPerBuffer = sampleRate / BUFFERS_PER_SECOND;
        this.isCapture = isCapture;
        this.nativePointer = nativePointer;
        byteBuffer = ByteBuffer.allocateDirect(bytesPerFrame * framesPerBuffer);
        final int channelConfig = channelCountToConfiguration(channels);
        if (isCapture) {
            int minBufferSize = AudioRecord.getMinBufferSize(sampleRate, channelConfig, DEFAULT_AUDIO_FORMAT);
            int bufferSizeInBytes = Math.max(BUFFER_SIZE_FACTOR * minBufferSize, byteBuffer.capacity());
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                audioRecord = createAudioRecordOnMOrHigher(
                        sampleRate,
                        channelConfig,
                        bufferSizeInBytes
                );
            } else {
                audioRecord = createAudioRecordOnLowerThanM(
                        sampleRate,
                        channelConfig,
                        bufferSizeInBytes
                );
            }
            effects.enable(audioRecord.getAudioSessionId());
        } else {
            final int minBufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRate, channelConfig, DEFAULT_AUDIO_FORMAT);

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                audioTrack = createAudioTrackOnOreoOrHigher(sampleRate, channelConfig, minBufferSizeInBytes);
            } else {
                audioTrack = createAudioTrackBeforeOreo(sampleRate, channelConfig, minBufferSizeInBytes);
            }
        }
    }

    private int channelCountToConfiguration(int channels) {
        return (channels == 1 ? AudioFormat.CHANNEL_IN_MONO : AudioFormat.CHANNEL_IN_STEREO);
    }

    public void release() {
        if (audioThread != null) {
            audioThread.stopThread();
            try {
                audioThread.join();
            } catch (InterruptedException ignored) {}
            audioThread = null;
        }
        if (audioRecord != null) {
            audioRecord.release();
            audioRecord = null;
        }
        if (audioTrack != null) {
            audioTrack.release();
            audioTrack = null;
        }
        effects.release();
        effects = null;
    }

    @TargetApi(26)
    private static AudioTrack createAudioTrackOnOreoOrHigher(int sampleRateInHz, int channelConfig, int bufferSizeInBytes) {
        return new AudioTrack.Builder()
                .setAudioAttributes(getAudioAttributes())
                .setAudioFormat(new AudioFormat.Builder()
                        .setEncoding(DEFAULT_AUDIO_FORMAT)
                        .setSampleRate(sampleRateInHz)
                        .setChannelMask(channelConfig)
                        .build())
                .setBufferSizeInBytes(bufferSizeInBytes)
                .setTransferMode(AudioTrack.MODE_STREAM)
                .build();
    }

    private static AudioTrack createAudioTrackBeforeOreo(int sampleRateInHz, int channelConfig, int bufferSizeInBytes) {
        return new AudioTrack(
                getAudioAttributes(),
                (new AudioFormat.Builder()).setEncoding(AudioFormat.ENCODING_PCM_16BIT).setSampleRate(sampleRateInHz).setChannelMask(channelConfig).build(),
                bufferSizeInBytes,
                1,
                0
        );
    }

    private static AudioAttributes getAudioAttributes() {
        return new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_VOICE_COMMUNICATION)
                .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
                .build();
    }

    @RequiresPermission(android.Manifest.permission.RECORD_AUDIO)
    private static AudioRecord createAudioRecordOnLowerThanM(int sampleRate, int channelConfig, int bufferSizeInBytes) {
        return new AudioRecord(MediaRecorder.AudioSource.VOICE_COMMUNICATION, sampleRate, channelConfig, DEFAULT_AUDIO_FORMAT, bufferSizeInBytes);
    }

    @TargetApi(Build.VERSION_CODES.M)
    @RequiresPermission(android.Manifest.permission.RECORD_AUDIO)
    private static AudioRecord createAudioRecordOnMOrHigher(int sampleRate, int channelConfig, int bufferSizeInBytes) {
        return new AudioRecord.Builder()
                .setAudioSource(android.media.MediaRecorder.AudioSource.VOICE_COMMUNICATION)
                .setAudioFormat(new AudioFormat.Builder()
                        .setEncoding(DEFAULT_AUDIO_FORMAT)
                        .setSampleRate(sampleRate)
                        .setChannelMask(channelConfig)
                        .build())
                .setBufferSizeInBytes(bufferSizeInBytes)
                .build();
    }

    public void open() {
        if (isCapture) {
            audioRecord.startRecording();
        } else {
            audioTrack.play();
        }
        audioThread = new AudioThread("AudioRecordJavaThread");
        audioThread.start();
    }

    private native void onRecordedData(byte[] data);

    private native void getPlaybackData();

    protected class AudioThread extends Thread {
        public volatile boolean keepAlive = true;
        private LowLatencyAudioBufferManager bufferManager;

        public AudioThread(String name) {
            super(name);
            if (!isCapture) {
                bufferManager = new LowLatencyAudioBufferManager();
            }
        }

        @Override
        public void run() {
            android.os.Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
            while (keepAlive) {
                if (isCapture) {
                    int bytesRead = audioRecord.read(byteBuffer, byteBuffer.capacity());
                    if (bytesRead == byteBuffer.capacity()) {
                        if (audioThread.keepAlive) {
                            onRecordedData(byteBuffer.array());
                        }
                    } else {
                        if (bytesRead == AudioRecord.ERROR_INVALID_OPERATION) {
                            audioThread.stopThread();
                        }
                    }
                } else {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                        bufferManager.maybeAdjustBufferSize(audioTrack);
                    }
                    getPlaybackData();
                    audioTrack.write(byteBuffer, byteBuffer.capacity(), AudioTrack.WRITE_BLOCKING);
                    byteBuffer.rewind();
                }
            }
            if (isCapture) {
                if (audioRecord != null) {
                    audioRecord.stop();
                }
            }
        }

        public void stopThread() {
            keepAlive = false;
        }
    }
}
