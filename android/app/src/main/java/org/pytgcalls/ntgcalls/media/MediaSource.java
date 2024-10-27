package org.pytgcalls.ntgcalls.media;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@Retention(RetentionPolicy.SOURCE)
@IntDef(flag=true, value={MediaSource.FILE, MediaSource.SHELL, MediaSource.FFMPEG, MediaSource.DEVICE, MediaSource.DESKTOP, MediaSource.EXTERNAL})
public @interface MediaSource {
    int FILE = 1;
    int SHELL = 1 << 1;
    int FFMPEG = 1 << 2;
    int DEVICE = 1 << 3;
    int DESKTOP = 1 << 4;
    int EXTERNAL = 1 << 5;
}