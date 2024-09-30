package org.pytgcalls.ntgcalls.devices;

import android.media.audiofx.AcousticEchoCanceler;
import android.media.audiofx.AudioEffect;
import android.media.audiofx.AudioEffect.Descriptor;
import android.media.audiofx.NoiseSuppressor;
import android.os.Build;

import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import java.util.UUID;

class AudioEffects {
    private static @Nullable Descriptor[] cachedEffects;
    private boolean shouldEnableAec;
    private boolean shouldEnableNs;
    private @Nullable AcousticEchoCanceler aec;
    private @Nullable NoiseSuppressor ns;

    private static final UUID AOSP_ACOUSTIC_ECHO_CANCELER =
            UUID.fromString("bb392ec0-8d4d-11e0-a896-0002a5d5c51b");
    private static final UUID AOSP_NOISE_SUPPRESSOR =
            UUID.fromString("c06c8400-8e06-11e0-9cb6-0002a5d5c51b");

    private static @Nullable Descriptor[] getAvailableEffects() {
        if (cachedEffects != null) {
            return cachedEffects;
        }
        cachedEffects = AudioEffect.queryEffects();
        return cachedEffects;
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public static boolean isAcousticEchoCancelerSupported() {
        return isEffectTypeAvailable(AudioEffect.EFFECT_TYPE_AEC, AOSP_ACOUSTIC_ECHO_CANCELER);
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public static boolean isNoiseSuppressorSupported() {
        return isEffectTypeAvailable(AudioEffect.EFFECT_TYPE_NS, AOSP_NOISE_SUPPRESSOR);
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public boolean setAEC(boolean enable) {
        if (!isAcousticEchoCancelerSupported()) {
            shouldEnableAec = false;
            return false;
        }
        if (aec != null && (enable != shouldEnableAec)) {
            return false;
        }
        shouldEnableAec = enable;
        return true;
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public boolean setNS(boolean enable) {
        if (!isNoiseSuppressorSupported()) {
            shouldEnableNs = false;
            return false;
        }
        if (ns != null && (enable != shouldEnableNs)) {
            return false;
        }
        shouldEnableNs = enable;
        return true;
    }

    protected void release() {
        if (aec != null) {
            aec.release();
            aec = null;
        }
        if (ns != null) {
            ns.release();
            ns = null;
        }
    }

    private static boolean isEffectTypeAvailable(UUID effectType, UUID blockListedUuid) {
        Descriptor[] effects = getAvailableEffects();
        if (effects == null) {
            return false;
        }
        for (Descriptor d : effects) {
            if (d.type.equals(effectType)) {
                return !d.uuid.equals(blockListedUuid);
            }
        }
        return false;
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public void enable(int audioSession) {
        if (isAcousticEchoCancelerSupported()) {
            aec = AcousticEchoCanceler.create(audioSession);
            if (aec != null) {
                boolean enable = shouldEnableAec && isAcousticEchoCancelerSupported();
                aec.setEnabled(enable);
            }
        }
        if (isNoiseSuppressorSupported()) {
            ns = NoiseSuppressor.create(audioSession);
            if (ns != null) {
                boolean enable = shouldEnableNs && isNoiseSuppressorSupported();
                ns.setEnabled(enable);
            }
        }
    }
}
