package org.pytgcalls.ntgcalls.media;

import java.util.List;


public class MediaDevices {
    public final List<DeviceInfo> audio;
    public final List<DeviceInfo> video;

    public MediaDevices(List<DeviceInfo> audio, List<DeviceInfo> video) {
        this.audio = audio;
        this.video = video;
    }
}
