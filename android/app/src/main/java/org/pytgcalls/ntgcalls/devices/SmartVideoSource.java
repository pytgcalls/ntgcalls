package org.pytgcalls.ntgcalls.devices;

import static org.webrtc.ApplicationContextProvider.getApplicationContext;

import android.content.Intent;
import android.media.projection.MediaProjection;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;

import org.pytgcalls.ntgcalls.AndroidUtils;
import org.webrtc.Camera1Enumerator;
import org.webrtc.Camera2Enumerator;
import org.webrtc.CameraEnumerator;
import org.webrtc.CameraVideoCapturer;
import org.webrtc.CapturerObserver;
import org.webrtc.EglBase;
import org.webrtc.ScreenCapturerAndroid;
import org.webrtc.SurfaceTextureHelper;
import org.webrtc.VideoCapturer;
import org.webrtc.VideoFrame;

import java.util.Objects;

class SmartVideoSource {
    private static final String TAG = "SmartVideoSource";

    private final static SmartVideoSource[] instance = new SmartVideoSource[2];

    public static Intent mediaProjectionPermissionResultData;
    private static EglBase eglBase;
    private HandlerThread thread;
    private Handler handler;
    private VideoCapturer videoCapturer;
    private boolean isRecording;
    private Runnable releaseRunnable;
    private SurfaceTextureHelper videoCapturerSurfaceTextureHelper;
    private final Object lock = new Object();
    private String deviceName;
    private final int width;
    private final int height;
    private final int fps;
    private final int id;
    private CapturerObserver capturerObserver;

    SmartVideoSource(String deviceName, int id, int width, int height, int fps) {
        this.width = width;
        this.height = height;
        this.fps = fps;
        this.deviceName = deviceName;
        this.id = id;
        AndroidUtils.runOnUIThread(() -> {
            if (thread == null) {
                thread = new HandlerThread("CallThread");
                thread.start();
                handler = new Handler(thread.getLooper());
            }
            setupDevice();
        });
    }

    private void setupDevice() {
        initSharedEGL();
        if ("screen".equals(deviceName)) {
            if (Build.VERSION.SDK_INT < 21) {
                return;
            }
            videoCapturer = new ScreenCapturerAndroid(mediaProjectionPermissionResultData, new MediaProjection.Callback() {});
            videoCapturerSurfaceTextureHelper = SurfaceTextureHelper.create("ScreenCapturerThread", eglBase.getEglBaseContext());
            handler.post(() -> {
                synchronized (lock) {
                    if (videoCapturerSurfaceTextureHelper == null) {
                        return;
                    }
                    videoCapturer.initialize(videoCapturerSurfaceTextureHelper, getApplicationContext(), getInternalObserver());
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
            String cameraName = names[index];
            if (videoCapturer == null) {
                videoCapturer = enumerator.createCapturer(cameraName, null);
                videoCapturerSurfaceTextureHelper = SurfaceTextureHelper.create("VideoCapturerThread", eglBase.getEglBaseContext());
                handler.post(() -> {
                    synchronized (lock) {
                        if (videoCapturerSurfaceTextureHelper == null) {
                            return;
                        }
                        videoCapturer.initialize(videoCapturerSurfaceTextureHelper, getApplicationContext(), getInternalObserver());
                    }
                });
            } else {
                handler.post(() -> ((CameraVideoCapturer) videoCapturer).switchCamera(null, cameraName));
            }
        }
    }

    public static SmartVideoSource getInstance(boolean isScreencast, String deviceName, int width, int height, int fps) {
        int index = isScreencast ? 1 : 0;
        boolean created = false;
        if (instance[index] == null) {
            instance[index] = new SmartVideoSource(deviceName, index, width, height, fps);
            created = true;
        }
        if (!created) {
            var checkInstance = instance[index];
            AndroidUtils.cancelRunOnUIThread(checkInstance.releaseRunnable);
            if (checkInstance.width != width || checkInstance.height != height || checkInstance.fps != fps || (Objects.equals(checkInstance.deviceName, "screen") && !Objects.equals(checkInstance.deviceName, deviceName))) {
                instance[index].release();
                instance[index] = new SmartVideoSource(deviceName, index, width, height, fps);
            } else {
                Log.d(TAG, "Reusing video source");
                instance[index].deviceName = deviceName;
                AndroidUtils.runOnUIThread(() -> instance[index].setupDevice());
            }
        }
        return instance[index];
    }

    public void open() {
        if (isRecording) {
            return;
        }
        isRecording = true;
        AndroidUtils.runOnUIThread(() -> handler.post(() -> {
            if (videoCapturer != null) {
                videoCapturer.startCapture(width, height, fps);
            }
        }));
    }

    private static void initSharedEGL() {
        if (eglBase == null) {
            eglBase = EglBase.create(null, EglBase.CONFIG_PLAIN);
        }
    }

    public static EglBase.Context getEglContext() {
        initSharedEGL();
        return eglBase.getEglBaseContext();
    }

    public void setCapturerObserver(CapturerObserver capturerObserver) {
        this.capturerObserver = capturerObserver;
    }

    private CapturerObserver getInternalObserver() {
        return new CapturerObserver() {
            @Override
            public void onCapturerStarted(boolean b) {}

            @Override
            public void onCapturerStopped() {
                synchronized (lock) {
                    if (videoCapturerSurfaceTextureHelper == null) {
                        return;
                    }
                    if (capturerObserver != null) {
                        capturerObserver.onCapturerStopped();
                    }
                }
            }

            @Override
            public void onFrameCaptured(VideoFrame videoFrame) {
                synchronized (lock) {
                    if (videoCapturerSurfaceTextureHelper == null) {
                        return;
                    }
                    if (capturerObserver != null) {
                        capturerObserver.onFrameCaptured(videoFrame);
                    }
                }
            }
        };
    }

    public void softRelease() {
        synchronized (lock) {
            Log.d(TAG, "Soft releasing video source");
            capturerObserver = null;
            releaseRunnable = this::release;
            AndroidUtils.runOnUIThread(releaseRunnable, 200);
        }
    }

    public void release() {
        synchronized (lock) {
            if (Build.VERSION.SDK_INT < 18) {
                return;
            }
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
            try {
                thread.quitSafely();
            } catch (Exception e) {
                Log.e("VideoCapturerModule", "release", e);
            }
            instance[id] = null;
        }
        Log.d(TAG, "Released video source");
    }
}
