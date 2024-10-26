package org.webrtc;

import android.graphics.SurfaceTexture;
import android.view.TextureView;

import androidx.annotation.NonNull;

import java.util.concurrent.CountDownLatch;

public class TextureEglRenderer extends EglRenderer implements TextureView.SurfaceTextureListener {
    private static final String TAG = "TextureEglRenderer";
    private RendererCommon.RendererEvents rendererEvents;
    final Object layoutLock = new Object();
    private boolean isFirstFrameRendered;
    private boolean isRenderingPaused;
    private int rotatedFrameWidth;
    private int rotatedFrameHeight;
    private int frameRotation;
    private int rotation;

    public TextureEglRenderer(String name) {
        super(name);
    }

    public void init(final EglBase.Context sharedContext,
                     RendererCommon.RendererEvents rendererEvents, final int[] configAttributes,
                     RendererCommon.GlDrawer drawer) {
        ThreadUtils.checkIsOnMainThread();
        this.rendererEvents = rendererEvents;
        synchronized (layoutLock) {
            isFirstFrameRendered = false;
            rotatedFrameWidth = 0;
            rotatedFrameHeight = 0;
            frameRotation = 0;
        }
        super.init(sharedContext, configAttributes, drawer);
    }

    @Override
    public void init(final EglBase.Context sharedContext, final int[] configAttributes, RendererCommon.GlDrawer drawer) {
        init(sharedContext, null, configAttributes, drawer);
    }

    @Override
    public void setFpsReduction(float fps) {
        synchronized (layoutLock) {
            isRenderingPaused = fps == 0f;
        }
        super.setFpsReduction(fps);
    }

    @Override
    public void disableFpsReduction() {
        synchronized (layoutLock) {
            isRenderingPaused = false;
        }
        super.disableFpsReduction();
    }

    @Override
    public void pauseVideo() {
        synchronized (layoutLock) {
            isRenderingPaused = true;
        }
        super.pauseVideo();
    }

    @Override
    public void onFrame(VideoFrame frame) {
        updateFrameDimensionsAndReportEvents(frame);
        frame = new VideoFrame(frame.getBuffer(), frame.getRotation() + rotation, frame.getTimestampNs());
        if (!isFirstFrameRendered) {
            isFirstFrameRendered = true;
            if (rendererEvents != null) {
                rendererEvents.onFirstFrameRendered();
            }
        }
        super.onFrame(frame);
    }

    @Override
    public void onSurfaceTextureAvailable(@NonNull SurfaceTexture surface, int width, int height) {
        ThreadUtils.checkIsOnMainThread();
        createEglSurface(surface);
    }

    @Override
    public void onSurfaceTextureSizeChanged(@NonNull SurfaceTexture surface, int width, int height) {
        ThreadUtils.checkIsOnMainThread();
        logD("surfaceChanged: size: " + width + "x" + height);
    }

    @Override
    public boolean onSurfaceTextureDestroyed(@NonNull SurfaceTexture surface) {
        ThreadUtils.checkIsOnMainThread();
        final CountDownLatch completionLatch = new CountDownLatch(1);
        releaseEglSurface(completionLatch::countDown);
        ThreadUtils.awaitUninterruptibly(completionLatch);
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(@NonNull SurfaceTexture surface) {

    }

    private void updateFrameDimensionsAndReportEvents(VideoFrame frame) {
        synchronized (layoutLock) {
            if (isRenderingPaused) {
                return;
            }
            if (rotatedFrameWidth != frame.getRotatedWidth()
                    || rotatedFrameHeight != frame.getRotatedHeight()
                    || frameRotation != frame.getRotation()) {
                logD("Reporting frame resolution changed to " + frame.getBuffer().getWidth() + "x"
                        + frame.getBuffer().getHeight() + " with rotation " + frame.getRotation());
                if (rendererEvents != null) {
                    rendererEvents.onFrameResolutionChanged(
                            frame.getBuffer().getWidth(), frame.getBuffer().getHeight(), frame.getRotation());
                }
                rotatedFrameWidth = frame.getRotatedWidth();
                rotatedFrameHeight = frame.getRotatedHeight();

                frameRotation = frame.getRotation();
            }
        }
    }

    private void logD(String string) {
        Logging.d(TAG, name + ": " + string);
    }

    public boolean isFirstFrameRendered() {
        return isFirstFrameRendered;
    }

    public void setRotation(int rotation) {
        synchronized (layoutLock) {
            this.rotation = rotation;
        }
    }
}
