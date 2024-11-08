package io.github.pytgcalls;

import androidx.annotation.RequiresPermission;

import io.github.pytgcalls.exceptions.ConnectionException;
import io.github.pytgcalls.exceptions.ConnectionNotFoundException;
import io.github.pytgcalls.media.FrameData;
import io.github.pytgcalls.media.MediaDescription;
import io.github.pytgcalls.media.MediaDevices;
import io.github.pytgcalls.media.MediaState;
import io.github.pytgcalls.media.MediaStatus;
import io.github.pytgcalls.media.SsrcGroup;
import io.github.pytgcalls.media.StreamDevice;
import io.github.pytgcalls.media.StreamMode;
import io.github.pytgcalls.p2p.AuthParams;
import io.github.pytgcalls.p2p.DhConfig;
import io.github.pytgcalls.p2p.Protocol;
import io.github.pytgcalls.p2p.RTCServer;

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

    @RequiresPermission(allOf = {android.Manifest.permission.RECORD_AUDIO, android.Manifest.permission.CAMERA})
    public native void createP2PCall(long chatId, MediaDescription mediaDescription) throws FileNotFoundException, ConnectionException;

    @RequiresPermission(allOf = {android.Manifest.permission.RECORD_AUDIO, android.Manifest.permission.CAMERA})
    public void createP2PCall(long chatId) throws FileNotFoundException, ConnectionException {
        createP2PCall(chatId, null);
    }

    public native byte[] initExchange(long chatId, DhConfig dhConfig, byte[] g_a_hash) throws ConnectionException;

    public native AuthParams exchangeKeys(long chatId, byte[] g_a_or_b, int keyFingerprint) throws ConnectionException;

    public native void skipExchange(long chatId, byte[] encryptionKey, boolean isOutgoing) throws ConnectionException;

    public native void connectP2P(long chatId, List<RTCServer> rtcServers, List<String> versions, boolean p2pAllowed) throws ConnectionException;

    @RequiresPermission(allOf = {android.Manifest.permission.RECORD_AUDIO, android.Manifest.permission.CAMERA})
    public native String createCall(long chatId, MediaDescription mediaDescription) throws FileNotFoundException, ConnectionException;

    @RequiresPermission(allOf = {android.Manifest.permission.RECORD_AUDIO, android.Manifest.permission.CAMERA})
    public String createCall(long chatId) throws FileNotFoundException, ConnectionException {
        return createCall(chatId, null);
    }

    public native void connect(long chatId, String params, boolean isPresentation) throws ConnectionException;

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

    public native void setFrameCallback(FrameCallback callback);

    public native void setRemoteSourceChangeCallback(RemoteSourceChangeCallback callback);

    public native void sendSignalingData(long chatId, byte[] data) throws ConnectionNotFoundException;

    public native void sendExternalFrame(long chatId, StreamDevice device, byte[] data, FrameData frameData);

    public native void addIncomingVideo(long chatId, String endpoint, List<SsrcGroup> ssrcGroups);

    public native void removeIncomingVideo(long chatId, String endpoint);

    public native Map<Long, MediaStatus> calls();
}
