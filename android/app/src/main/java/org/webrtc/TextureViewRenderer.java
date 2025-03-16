package org.webrtc;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.os.Looper;
import android.view.TextureView;
import android.view.View;

import androidx.annotation.NonNull;

import io.github.pytgcalls.AndroidUtils;

public class TextureViewRenderer extends TextureView implements TextureView.SurfaceTextureListener, VideoSink, RendererCommon.RendererEvents {
    private static final String TAG = "TextureViewRenderer";
    private final String resourceName;
    private boolean isCamera;
    private boolean mirror;
    private boolean useCameraRotation;
    private boolean rotateTextureWithScreen;
    private boolean enableFixedSize;
    private final TextureEglRenderer eglRenderer;
    private OrientationHelper orientationHelper;
    private int rotatedFrameWidth;
    private int rotatedFrameHeight;
    private int textureRotation;
    private int screenRotation;
    private int surfaceWidth;
    private int surfaceHeight;
    private int videoWidth;
    private int videoHeight;
    private int maxTextureSize;
    Runnable updateScreenRunnable;
    private final RendererCommon.VideoLayoutMeasure videoLayoutMeasure = new RendererCommon.VideoLayoutMeasure();
    private RendererCommon.RendererEvents rendererEvents;

    public TextureViewRenderer(@NonNull Context context) {
        super(context);
        this.resourceName = getResourceName();
        eglRenderer = new TextureEglRenderer(resourceName);
        setSurfaceTextureListener(this);
    }

    @Override
    public void onSurfaceTextureAvailable(@NonNull SurfaceTexture surface, int width, int height) {
        ThreadUtils.checkIsOnMainThread();
        surfaceWidth = surfaceHeight = 0;
        updateSurfaceSize();
        eglRenderer.onSurfaceTextureAvailable(surface, width, height);
    }

    @Override
    public void onSurfaceTextureSizeChanged(@NonNull SurfaceTexture surface, int width, int height) {
        surfaceWidth = width;
        surfaceHeight = height;
        eglRenderer.onSurfaceTextureSizeChanged(surface, width, height);
    }

    @Override
    public boolean onSurfaceTextureDestroyed(@NonNull SurfaceTexture surface) {
        eglRenderer.onSurfaceTextureDestroyed(surface);
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(@NonNull SurfaceTexture surface) {
        eglRenderer.onSurfaceTextureUpdated(surface);
    }

    @Override
    public void onFirstFrameRendered() {
        if (rendererEvents != null) {
            rendererEvents.onFirstFrameRendered();
        }
    }

    @Override
    public void onFrameResolutionChanged(int videoWidth, int videoHeight, int rotation) {
        if (rendererEvents != null) {
            rendererEvents.onFrameResolutionChanged(videoWidth, videoHeight, rotation);
        }
        textureRotation = rotation;
        int rotatedWidth, rotatedHeight;

        if (rotateTextureWithScreen) {
            if (isCamera) {
                onRotationChanged();
            }
            if (useCameraRotation) {
                rotatedWidth = screenRotation == 0 ? videoHeight : videoWidth;
                rotatedHeight = screenRotation == 0 ? videoWidth : videoHeight;
            } else {
                rotatedWidth = textureRotation == 0 || textureRotation == 180 || textureRotation == -180 ? videoWidth : videoHeight;
                rotatedHeight = textureRotation == 0 || textureRotation == 180 || textureRotation == -180 ? videoHeight : videoWidth;
            }
        } else {
            if (isCamera) {
                eglRenderer.setRotation(-OrientationHelper.cameraRotation);
            }
            rotation -= OrientationHelper.cameraOrientation;
            rotatedWidth = rotation == 0 || rotation == 180 || rotation == -180 ? videoWidth : videoHeight;
            rotatedHeight = rotation == 0 || rotation == 180 || rotation == -180? videoHeight : videoWidth;
        }
        synchronized (eglRenderer.layoutLock) {
            if (updateScreenRunnable != null) {
                AndroidUtils.cancelRunOnUIThread(updateScreenRunnable);
            }
            postOrRun(updateScreenRunnable = () -> {
                updateScreenRunnable = null;
                this.videoWidth = videoWidth;
                this.videoHeight = videoHeight;

                rotatedFrameWidth = rotatedWidth;
                rotatedFrameHeight = rotatedHeight;

                updateSurfaceSize();
                requestLayout();
            });
        }
    }

    public void init(EglBase.Context sharedContext, RendererCommon.RendererEvents rendererEvents) {
        init(sharedContext, rendererEvents, EglBase.CONFIG_PLAIN, new GlRectDrawer());
    }

    public void init(final EglBase.Context sharedContext, RendererCommon.RendererEvents rendererEvents, final int[] configAttributes, RendererCommon.GlDrawer drawer) {
        ThreadUtils.checkIsOnMainThread();
        this.rendererEvents = rendererEvents;
        rotatedFrameWidth = 0;
        rotatedFrameHeight = 0;
        eglRenderer.init(sharedContext, this, configAttributes, drawer);
    }

    @Override
    public void onFrame(VideoFrame videoFrame) {
        eglRenderer.onFrame(videoFrame);
    }

    public void release() {
        eglRenderer.release();
        if (orientationHelper != null) {
            orientationHelper.stop();
        }
    }

    public void setIsCamera(boolean value) {
        isCamera = value;
        if (!isCamera) {
            orientationHelper = new OrientationHelper() {
                @Override
                protected void onOrientationUpdate(int orientation) {
                    if (!isCamera) {
                        updateRotation();
                    }
                }
            };
            orientationHelper.start();
        }
    }

    public void updateRotation() {
        if (orientationHelper == null || rotatedFrameWidth == 0 || rotatedFrameHeight == 0) {
            return;
        }
        View parentView = (View) getParent();
        if (parentView == null) {
            return;
        }
        int orientation = orientationHelper.getOrientation();
        float viewWidth = getMeasuredWidth();
        float viewHeight = getMeasuredHeight();
        float w;
        float h;
        float targetWidth = parentView.getMeasuredWidth();
        float targetHeight = parentView.getMeasuredHeight();
        if (orientation == 90 || orientation == 270) {
            w = viewHeight;
            h = viewWidth;
        } else {
            w = viewWidth;
            h = viewHeight;
        }
        float scale;
        if (w < h) {
            scale = Math.max(w / viewWidth, h / viewHeight);
        } else {
            scale = Math.min(w / viewWidth, h / viewHeight);
        }
        w *= scale;
        h *= scale;
        if (Math.abs(w / h - targetWidth / targetHeight) < 0.1f) {
            scale *= Math.max(targetWidth / w, targetHeight / h);
        }
        if (orientation == 270) {
            orientation = -90;
        }
        animate().scaleX(scale).scaleY(scale).rotation(-orientation).setDuration(180).start();
    }

    private String getResourceName() {
        try {
            return getResources().getResourceEntryName(getId());
        } catch (Resources.NotFoundException e) {
            return "";
        }
    }

    public void setUseCameraRotation(boolean useCameraRotation) {
        if (this.useCameraRotation != useCameraRotation) {
            this.useCameraRotation = useCameraRotation;
            onRotationChanged();
            updateVideoSizes();
        }
    }

    private void postOrRun(Runnable r) {
        if (Thread.currentThread() == Looper.getMainLooper().getThread()) {
            r.run();
        } else {
            AndroidUtils.runOnUIThread(r);
        }
    }

    private void onRotationChanged() {
        int rotation = useCameraRotation ? OrientationHelper.cameraOrientation : 0;
        if (mirror) {
            rotation = 360 - rotation;
        }
        int r = -rotation;
        if (useCameraRotation) {
            if (screenRotation == 1) {
                r += mirror ? 90 : -90;
            } else if (screenRotation == 3) {
                r += mirror ? 270 : -270;
            }
        }
        eglRenderer.setRotation(r);
        eglRenderer.setMirror(mirror);
    }

    private void updateSurfaceSize() {
        ThreadUtils.checkIsOnMainThread();
        if (enableFixedSize && rotatedFrameWidth != 0 && rotatedFrameHeight != 0 && getWidth() != 0
                && getHeight() != 0) {
            final float layoutAspectRatio = getWidth() / (float) getHeight();
            final float frameAspectRatio = rotatedFrameWidth / (float) rotatedFrameHeight;
            final int drawnFrameWidth;
            final int drawnFrameHeight;
            if (frameAspectRatio > layoutAspectRatio) {
                drawnFrameWidth = (int) (rotatedFrameHeight * layoutAspectRatio);
                drawnFrameHeight = rotatedFrameHeight;
            } else {
                drawnFrameWidth = rotatedFrameWidth;
                drawnFrameHeight = (int) (rotatedFrameHeight / layoutAspectRatio);
            }
            final int width = Math.min(getWidth(), drawnFrameWidth);
            final int height = Math.min(getHeight(), drawnFrameHeight);
            logD("updateSurfaceSize. Layout size: " + getWidth() + "x" + getHeight() + ", frame size: "  + rotatedFrameWidth + "x" + rotatedFrameHeight + ", requested surface size: " + width  + "x" + height + ", old surface size: " + surfaceWidth + "x" + surfaceHeight);
            if (width != surfaceWidth || height != surfaceHeight) {
                surfaceWidth = width;
                surfaceHeight = height;
            }
        } else {
            surfaceWidth = surfaceHeight = 0;
        }
    }

    private void logD(String string) {
        Logging.d(TAG, resourceName + ": " + string);
    }

    private void updateVideoSizes() {
        if (videoHeight != 0 && videoWidth != 0) {
            int rotatedWidth;
            int rotatedHeight;
            if (rotateTextureWithScreen) {
                if (useCameraRotation) {
                    rotatedWidth = screenRotation == 0 ? videoHeight : videoWidth;
                    rotatedHeight = screenRotation == 0 ? videoWidth : videoHeight;
                } else {
                    rotatedWidth = textureRotation == 0 || textureRotation == 180 || textureRotation == -180 ? videoWidth : videoHeight;
                    rotatedHeight = textureRotation == 0 || textureRotation == 180 || textureRotation == -180 ? videoHeight : videoWidth;
                }
            } else {
                int rotation = textureRotation;
                rotation -= OrientationHelper.cameraOrientation;
                rotatedWidth = rotation == 0 || rotation == 180 || rotation == -180 ? videoWidth : videoHeight;
                rotatedHeight = rotation == 0 || rotation == 180 || rotation == -180 ? videoHeight : videoWidth;
            }
            if (rotatedFrameWidth != rotatedWidth || rotatedFrameHeight != rotatedHeight) {
                synchronized (eglRenderer.layoutLock) {
                    if (updateScreenRunnable != null) {
                        AndroidUtils.cancelRunOnUIThread(updateScreenRunnable);
                    }
                    postOrRun(updateScreenRunnable = () -> {
                        updateScreenRunnable = null;

                        rotatedFrameWidth = rotatedWidth;
                        rotatedFrameHeight = rotatedHeight;

                        updateSurfaceSize();
                        requestLayout();
                    });
                }
            }
        }
    }

    @Override
    protected void onMeasure(int widthSpec, int heightSpec) {
        ThreadUtils.checkIsOnMainThread();
        if (!isCamera && rotateTextureWithScreen) {
            updateVideoSizes();
        }
        Point size;
        if (maxTextureSize > 0) {
            size = videoLayoutMeasure.measure(MeasureSpec.makeMeasureSpec(Math.min(maxTextureSize, MeasureSpec.getSize(widthSpec)), MeasureSpec.getMode(widthSpec)), MeasureSpec.makeMeasureSpec(Math.min(maxTextureSize, MeasureSpec.getSize(heightSpec)), MeasureSpec.getMode(heightSpec)), rotatedFrameWidth, rotatedFrameHeight);
        } else {
            size = videoLayoutMeasure.measure(widthSpec, heightSpec, rotatedFrameWidth, rotatedFrameHeight);
        }
        setMeasuredDimension(size.x, size.y);
        if (rotatedFrameWidth != 0 && rotatedFrameHeight != 0) {
            eglRenderer.setLayoutAspectRatio(getMeasuredWidth() / (float) getMeasuredHeight());
        }
        updateSurfaceSize();
    }

    public void setMaxTextureSize(int maxTextureSize) {
        this.maxTextureSize = maxTextureSize;
    }

    public void setScreenRotation(int screenRotation) {
        this.screenRotation = screenRotation;
        onRotationChanged();
        updateVideoSizes();
    }

    public void setRotateTextureWithScreen(boolean rotateTextureWithScreen) {
        if (this.rotateTextureWithScreen != rotateTextureWithScreen) {
            this.rotateTextureWithScreen = rotateTextureWithScreen;
            requestLayout();
        }
    }

    public void setEnableHardwareScaler(boolean enabled) {
        ThreadUtils.checkIsOnMainThread();
        enableFixedSize = enabled;
        updateSurfaceSize();
    }

    public void setMirror(final boolean mirror) {
        if (this.mirror != mirror) {
            this.mirror = mirror;
            if (rotateTextureWithScreen) {
                onRotationChanged();
            } else {
                eglRenderer.setMirror(mirror);
            }
            updateSurfaceSize();
            requestLayout();
        }
    }

    public void setScalingType(RendererCommon.ScalingType scalingType) {
        ThreadUtils.checkIsOnMainThread();
        videoLayoutMeasure.setScalingType(scalingType);
        requestLayout();
    }

    public void setFpsReduction(float fps) {
        eglRenderer.setFpsReduction(fps);
    }
}
