package io.github.pytgcalls.media;

import java.util.List;


public class MediaDevices {
    public final List<DeviceInfo> audio;
    public final List<DeviceInfo> video;
    public final List<DeviceInfo> screen;

    public MediaDevices(List<DeviceInfo> audio, List<DeviceInfo> video, List<DeviceInfo> screen) {
        this.audio = audio;
        this.video = video;
        this.screen = screen;
    }
}
