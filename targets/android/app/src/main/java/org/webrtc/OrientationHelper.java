package org.webrtc;

import static org.webrtc.ApplicationContextProvider.getApplicationContext;

import android.view.OrientationEventListener;
public class OrientationHelper {

    private static final int ORIENTATION_HYSTERESIS = 5;
    private OrientationEventListener orientationEventListener;
    private int rotation;

    public static volatile boolean cameraRotationDisabled;
    public static volatile int cameraRotation;
    public static volatile int cameraOrientation;

    private int roundOrientation(int orientation, int orientationHistory) {
        boolean changeOrientation;
        if (orientationHistory == OrientationEventListener.ORIENTATION_UNKNOWN) {
            changeOrientation = true;
        } else {
            int dist = Math.abs(orientation - orientationHistory);
            dist = Math.min(dist, 360 - dist);
            changeOrientation = (dist >= 45 + ORIENTATION_HYSTERESIS);
        }
        if (changeOrientation) {
            return ((orientation + 45) / 90 * 90) % 360;
        }
        return orientationHistory;
    }

    public OrientationHelper() {
        orientationEventListener = new OrientationEventListener(getApplicationContext()) {
            @Override
            public void onOrientationChanged(int orientation) {
                if (orientationEventListener == null || orientation == ORIENTATION_UNKNOWN) {
                    return;
                }
                int newOrientation = roundOrientation(orientation, rotation);
                if (newOrientation != rotation) {
                    onOrientationUpdate(rotation = newOrientation);
                }
            }
        };
    }

    protected void onOrientationUpdate(int orientation) {

    }

    public void start() {
        if (orientationEventListener.canDetectOrientation()) {
            orientationEventListener.enable();
        } else {
            orientationEventListener.disable();
            orientationEventListener = null;
        }
    }

    public void stop() {
        if (orientationEventListener != null) {
            orientationEventListener.disable();
            orientationEventListener = null;
        }
    }

    public int getOrientation() {
        if (cameraRotationDisabled) {
            return 0;
        }
        return rotation;
    }
}
