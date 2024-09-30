package org.pytgcalls.ntgcalls;

import androidx.annotation.RequiresPermission;

import org.pytgcalls.ntgcalls.exceptions.ConnectionException;
import org.pytgcalls.ntgcalls.exceptions.ConnectionNotFoundException;
import org.pytgcalls.ntgcalls.media.MediaDescription;
import org.pytgcalls.ntgcalls.media.MediaDevices;
import org.pytgcalls.ntgcalls.media.MediaState;
import org.pytgcalls.ntgcalls.media.MediaStatus;
import org.pytgcalls.ntgcalls.media.StreamMode;
import org.pytgcalls.ntgcalls.p2p.AuthParams;
import org.pytgcalls.ntgcalls.p2p.DhConfig;
import org.pytgcalls.ntgcalls.p2p.Protocol;
import org.pytgcalls.ntgcalls.p2p.RTCServer;

import java.io.FileNotFoundException;
import java.util.List;
import java.util.Map;

public class NTgCalls {
    @SuppressWarnings("unused")
    private long nativePointer;

    static {
        System.loadLibrary("ntgcalls");
    }

    private native void init();

    public NTgCalls() {
        init();
    }

    private native void destroy();

    @Override
    protected void finalize() throws Throwable {
        try {
            destroy();
        } finally {
            super.finalize();
        }
    }

    private static native String pingNative();

    @RequiresPermission(android.Manifest.permission.RECORD_AUDIO)
    public native void createP2PCall(long chatId, MediaDescription mediaDescription) throws FileNotFoundException, ConnectionException;

    @RequiresPermission(android.Manifest.permission.RECORD_AUDIO)
    public void createP2PCall(long chatId) throws FileNotFoundException, ConnectionException {
        createP2PCall(chatId, null);
    }

    public native byte[] initExchange(long chatId, DhConfig dhConfig, byte[] g_a_hash) throws ConnectionException;

    public native AuthParams exchangeKeys(long chatId, byte[] g_a_or_b, int keyFingerprint) throws ConnectionException;

    public native void skipExchange(long chatId, byte[] encryptionKey, boolean isOutgoing) throws ConnectionException;

    public native void connectP2P(long chatId, List<RTCServer> rtcServers, List<String> versions, boolean p2pAllowed) throws ConnectionException;

    @RequiresPermission(android.Manifest.permission.RECORD_AUDIO)
    public native String createCall(long chatId, MediaDescription mediaDescription) throws FileNotFoundException, ConnectionException;

    @RequiresPermission(android.Manifest.permission.RECORD_AUDIO)
    public String createCall(long chatId) throws FileNotFoundException, ConnectionException {
        return createCall(chatId, null);
    }

    public native void connect(long chatId, String params) throws ConnectionException;

    @RequiresPermission(android.Manifest.permission.RECORD_AUDIO)
    public native void setStreamSources(long chatId, StreamMode mode, MediaDescription mediaDescription) throws FileNotFoundException, ConnectionNotFoundException;

    public native boolean pause(long chatId) throws ConnectionNotFoundException;

    public native boolean resume(long chatId) throws ConnectionNotFoundException;

    public native boolean mute(long chatId) throws ConnectionNotFoundException;

    public native boolean unmute(long chatId) throws ConnectionNotFoundException;

    public native void stop(long chatId) throws ConnectionNotFoundException;

    public native long time(long chatId, StreamMode mode) throws ConnectionNotFoundException;

    public native MediaState getState(long chatId) throws ConnectionNotFoundException;

    public static native Protocol getProtocol();

    public static native MediaDevices getMediaDevices();

    public static long ping() {
        var startTime = System.currentTimeMillis();
        pingNative();
        return System.currentTimeMillis() - startTime;
    }

    public native void setUpgradeCallback(UpgradeCallback callback);

    public native void setStreamEndCallback(StreamEndCallback callback);

    public native void setConnectionChangeCallback(ConnectionChangeCallback callback);

    public native void setSignalingDataCallback(SignalingDataCallback callback);

    public native void sendSignalingData(long chatId, byte[] data) throws ConnectionNotFoundException;

    public native Map<Long, MediaStatus> calls();
}
