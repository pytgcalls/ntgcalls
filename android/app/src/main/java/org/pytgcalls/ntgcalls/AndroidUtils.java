package org.pytgcalls.ntgcalls;

import android.os.Handler;

import org.webrtc.ApplicationContextProvider;

public class AndroidUtils {
    private static Handler appHandler;

    public static void runOnUIThread(Runnable runnable) {
        if (appHandler == null) {
            appHandler = new Handler(ApplicationContextProvider.getApplicationContext().getMainLooper());
        }
        appHandler.post(runnable);
    }

    public static void cancelRunOnUIThread(Runnable runnable) {
        if (appHandler == null) {
            return;
        }
        appHandler.removeCallbacks(runnable);
    }
}
