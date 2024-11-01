package io.github.pytgcalls.devices;

import static org.webrtc.ApplicationContextProvider.getApplicationContext;

import android.content.Intent;

import org.json.JSONException;
import org.json.JSONObject;
import io.github.pytgcalls.media.DeviceInfo;
import org.webrtc.Camera1Enumerator;
import org.webrtc.Camera2Enumerator;
import org.webrtc.CameraEnumerator;
import org.webrtc.CapturerObserver;
import org.webrtc.EglBase;
import org.webrtc.VideoFrame;

import java.util.ArrayList;
import java.util.List;

@SuppressWarnings("unused")
public class JavaVideoCapturerModule {
    private final SmartVideoSource instance;
    private final Object lock = new Object();

    @SuppressWarnings({"unused", "FieldCanBeLocal"})
    private long nativePointer;

    public JavaVideoCapturerModule(boolean isScreencast, String deviceName, int width, int height, int fps, long nativePointer) {
        this.nativePointer = nativePointer;
        instance = SmartVideoSource.getInstance(isScreencast, deviceName, width, height, fps);
        instance.setCapturerObserver(new CapturerObserver() {
            @Override
            public void onCapturerStarted(boolean b) {}

            @Override
            public void onCapturerStopped() {
                if (nativePointer == 0) {
                    return;
                }
                synchronized (lock) {
                    nativeCapturerStopped();
                }
            }

            @Override
            public void onFrameCaptured(VideoFrame videoFrame) {
                if (nativePointer == 0) {
                    return;
                }
                synchronized (lock) {
                    nativeOnFrame(videoFrame);
                }
            }
        });
    }

    private static List<DeviceInfo> getDevices() throws JSONException {
        CameraEnumerator enumerator = Camera2Enumerator.isSupported(getApplicationContext()) ? new Camera2Enumerator(getApplicationContext()) : new Camera1Enumerator();
        String[] names = enumerator.getDeviceNames();
        List<DeviceInfo> devices = new ArrayList<>(names.length);
        for (String name : names) {
            boolean isFrontFace = enumerator.isFrontFacing(name);
            JSONObject jsonObject = new JSONObject();
            jsonObject.put("id", name);
            jsonObject.put("is_front", isFrontFace);
            devices.add(new DeviceInfo(name, jsonObject.toString()));
        }
        return devices;
    }

    private void open() {
       synchronized (lock) {
           instance.open();
       }
    }

    private void release() {
        synchronized (lock) {
            nativePointer = 0;
            instance.softRelease();
        }
    }

    public static EglBase.Context getSharedEGLContext() {
        return SmartVideoSource.getEglContext();
    }

    public static void setMediaProjectionPermissionResult(Intent data) {
        SmartVideoSource.mediaProjectionPermissionResultData = data;
    }

    private native void nativeCapturerStopped();

    private native void nativeOnFrame(VideoFrame jFrame);
}
