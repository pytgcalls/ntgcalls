package org.pytgcalls.ntgcalls;

import org.pytgcalls.ntgcalls.media.MediaDescription;

public class NTgCalls {
    @SuppressWarnings("unused")
    private long nativePointer;

    static {
        System.loadLibrary("ntgcalls");
    }

    private native void init();

    private native void destroy();

    private static native String pingNative();

    public native String createCall(long chatId, MediaDescription mediaDescription);

    public NTgCalls() {
        init();
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            destroy();
        } finally {
            super.finalize();
        }
    }

    public static long ping() {
        var startTime = System.currentTimeMillis();
        pingNative();
        return System.currentTimeMillis() - startTime;
    }
}
