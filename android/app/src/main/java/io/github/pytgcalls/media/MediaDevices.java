package io.github.pytgcalls.media;

import java.util.List;


public class MediaDevices {
    public final List<DeviceInfo> microphone;
    public final List<DeviceInfo> speaker;
    public final List<DeviceInfo> camera;
    public final List<DeviceInfo> screen;

    public MediaDevices(List<DeviceInfo> microphone, List<DeviceInfo> speaker, List<DeviceInfo> camera, List<DeviceInfo> screen) {
        this.microphone = microphone;
        this.speaker = speaker;
        this.camera = camera;
        this.screen = screen;
    }
}
