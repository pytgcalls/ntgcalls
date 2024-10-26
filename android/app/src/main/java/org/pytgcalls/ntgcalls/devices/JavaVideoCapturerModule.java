package org.pytgcalls.ntgcalls.devices;

import static org.webrtc.ApplicationContextProvider.getApplicationContext;

import android.content.Intent;
import android.media.projection.MediaProjection;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;
import org.pytgcalls.ntgcalls.AndroidUtils;
import org.pytgcalls.ntgcalls.media.DeviceInfo;
import org.webrtc.Camera1Enumerator;
import org.webrtc.Camera2Enumerator;
import org.webrtc.CameraEnumerator;
import org.webrtc.CapturerObserver;
import org.webrtc.EglBase;
import org.webrtc.ScreenCapturerAndroid;
import org.webrtc.SurfaceTextureHelper;
import org.webrtc.VideoCapturer;
import org.webrtc.VideoFrame;

import java.util.ArrayList;
import java.util.List;

@SuppressWarnings("unused")
public class JavaVideoCapturerModule {
    private static final JavaVideoCapturerModule[] instance = new JavaVideoCapturerModule[2];
    public static EglBase eglBase;
    private HandlerThread thread;
    private Handler handler;
    private VideoCapturer videoCapturer;
    private SurfaceTextureHelper videoCapturerSurfaceTextureHelper;
    public static Intent mediaProjectionPermissionResultData;
    private final Object lock = new Object();
    private final int width;
    private final int height;
    private final int fps;

    @SuppressWarnings({"unused", "FieldCanBeLocal"})
    private long nativePointer;

    public JavaVideoCapturerModule(boolean isScreencast, String deviceName, int width, int height, int fps, long nativePointer) {
        this.nativePointer = nativePointer;
        this.width = width;
        this.height = height;
        this.fps = fps;
        AndroidUtils.runOnUIThread(() -> {
            if (eglBase == null) {
                eglBase = EglBase.create(null, EglBase.CONFIG_PLAIN);
            }
            instance[isScreencast ? 1 : 0] = this;
            thread = new HandlerThread("CallThread");
            thread.start();
            handler = new Handler(thread.getLooper());
            setupDevice(deviceName);
        });
    }

    private void setupDevice(String deviceName) {
        if (eglBase == null) {
            return;
        }
        if ("screen".equals(deviceName)) {
            if (Build.VERSION.SDK_INT < 21) {
                return;
            }
            videoCapturer = new ScreenCapturerAndroid(mediaProjectionPermissionResultData, new MediaProjection.Callback() {});
            videoCapturerSurfaceTextureHelper = SurfaceTextureHelper.create("ScreenCapturerThread", eglBase.getEglBaseContext());
            handler.post(() -> {
                synchronized (lock) {
                    if (videoCapturerSurfaceTextureHelper == null || nativePointer == 0) {
                        return;
                    }
                    videoCapturer.initialize(videoCapturerSurfaceTextureHelper, getApplicationContext(), getNativeCapturer());
                }
            });
        } else {
            CameraEnumerator enumerator = Camera2Enumerator.isSupported(getApplicationContext()) ? new Camera2Enumerator(getApplicationContext()) : new Camera1Enumerator();
            int index = -1;
            String[] names = enumerator.getDeviceNames();
            for (int a = 0; a < names.length; a++) {
                if (names[a].equals(deviceName)) {
                    index = a;
                    break;
                }
            }
            if (index == -1) {
                return;
            }
            videoCapturer = enumerator.createCapturer(names[index], null);
            videoCapturerSurfaceTextureHelper = SurfaceTextureHelper.create("VideoCapturerThread", eglBase.getEglBaseContext());
            handler.post(() -> {
                synchronized (lock) {
                    if (videoCapturerSurfaceTextureHelper == null || nativePointer == 0) {
                        return;
                    }
                    videoCapturer.initialize(videoCapturerSurfaceTextureHelper, getApplicationContext(), getNativeCapturer());
                }
            });
        }
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
        AndroidUtils.runOnUIThread(() -> handler.post(() -> {
            if (videoCapturer != null) {
                videoCapturer.startCapture(width, height, fps);
            }
        }));
    }

    private CapturerObserver getNativeCapturer() {
        return new CapturerObserver() {
            @Override
            public void onCapturerStarted(boolean b) {}

            @Override
            public void onCapturerStopped() {
                synchronized (lock) {
                    if (videoCapturerSurfaceTextureHelper == null || nativePointer == 0) {
                        return;
                    }
                    nativeCapturerStopped();
                }
            }

            @Override
            public void onFrameCaptured(VideoFrame videoFrame) {
                synchronized (lock) {
                    if (videoCapturerSurfaceTextureHelper == null || nativePointer == 0) {
                        return;
                    }
                    nativeOnFrame(videoFrame);
                }
            }
        };
    }

    private void release() {
        synchronized (lock) {
            nativePointer = 0;
            if (Build.VERSION.SDK_INT < 18) {
                return;
            }
            AndroidUtils.runOnUIThread(() -> {
                for (int a = 0; a < instance.length; a++) {
                    if (instance[a] == this) {
                        instance[a] = null;
                        break;
                    }
                }
                handler.post(() -> {
                    if (videoCapturer != null) {
                        try {
                            videoCapturer.stopCapture();
                        } catch (InterruptedException e) {
                            Log.e("VideoCapturerModule", "release", e);
                        }
                        videoCapturer.dispose();
                        videoCapturer = null;
                    }
                    if (videoCapturerSurfaceTextureHelper != null) {
                        videoCapturerSurfaceTextureHelper.dispose();
                        videoCapturerSurfaceTextureHelper = null;
                    }
                });
                try {
                    thread.quitSafely();
                } catch (Exception e) {
                    Log.e("VideoCapturerModule", "release", e);
                }
            });
        }
    }

    public static EglBase.Context getSharedEGLContext() {
        if (eglBase == null) {
            eglBase = EglBase.create(null, EglBase.CONFIG_PLAIN);
        }
        return eglBase.getEglBaseContext();
    }

    private native void nativeCapturerStopped();

    private native void nativeOnFrame(VideoFrame jFrame);
}
