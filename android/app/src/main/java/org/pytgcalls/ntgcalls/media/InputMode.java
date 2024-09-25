package org.pytgcalls.ntgcalls.media;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@Retention(RetentionPolicy.SOURCE)
@IntDef(flag=true, value={InputMode.FILE, InputMode.SHELL, InputMode.FFMPEG, InputMode.NO_LATENCY, InputMode.DEVICE})
public @interface InputMode {
    int FILE = 1;
    int SHELL = 1 << 1;
    int FFMPEG = 1 << 2;
    int NO_LATENCY = 1 << 3;
    int DEVICE = 1 << 4;
}