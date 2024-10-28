package org.pytgcalls.ntgcalls;

import static org.webrtc.ApplicationContextProvider.getApplicationContext;

import android.os.Handler;

public class AndroidUtils {
    private static Handler appHandler;

    public static void runOnUIThread(Runnable runnable) {
        runOnUIThread(runnable, 0);
    }

    public static void cancelRunOnUIThread(Runnable runnable) {
        if (appHandler == null) {
            return;
        }
        appHandler.removeCallbacks(runnable);
    }

    public static void runOnUIThread(Runnable runnable, long delay) {
        if (appHandler == null) {
            var context = getApplicationContext();
            if (context == null) {
                return;
            }
            appHandler = new Handler(context.getMainLooper());
        }
        if (delay == 0) {
            appHandler.post(runnable);
        } else {
            appHandler.postDelayed(runnable, delay);
        }
    }
}
