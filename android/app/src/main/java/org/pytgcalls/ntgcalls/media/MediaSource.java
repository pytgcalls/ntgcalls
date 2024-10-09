package org.pytgcalls.ntgcalls.media;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@Retention(RetentionPolicy.SOURCE)
@IntDef(flag=true, value={MediaSource.FILE, MediaSource.SHELL, MediaSource.FFMPEG, MediaSource.DEVICE})
public @interface MediaSource {
    int FILE = 1;
    int SHELL = 1 << 1;
    int FFMPEG = 1 << 2;
    int DEVICE = 1 << 3;
}