import android.util.Log;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import static org.junit.Assert.fail;
import org.junit.runner.RunWith;
import io.github.pytgcalls.NTgCalls;
import io.github.pytgcalls.exceptions.ConnectionException;
import io.github.pytgcalls.exceptions.InvalidParamsException;
import io.github.pytgcalls.exceptions.TelegramServerException;
import io.github.pytgcalls.media.AudioDescription;
import io.github.pytgcalls.media.MediaDescription;
import io.github.pytgcalls.media.MediaDevices;
import io.github.pytgcalls.media.MediaSource;

import java.io.FileNotFoundException;

@RunWith(AndroidJUnit4.class)
public class NTgCallsTests {
    private static final int CHAT_ID = 1234567890;

    @Test
    public void ping() {
        Log.d("NTGCALLS", String.format("Ping Time: %d", NTgCalls.ping()));
    }

    @Test
    public void crashWithFileNotFound() {
        var instance = new NTgCalls();
        try {
            instance.createCall(
                    CHAT_ID,
                    new MediaDescription(
                            new AudioDescription(
                                    MediaSource.FILE,
                                    "input",
                                    48000,
                                    16,
                                    2
                            ),
                            null,
                            null,
                            null
                    )
            );
            fail("Expected an FileNotFoundException to be thrown");
        } catch (FileNotFoundException ignored) {
        } catch (ConnectionException e) {
            fail("Unexpected ConnectionException");
        }
    }

    @Test
    public void createCall() {
        var instance = new NTgCalls();
        try {
            Log.d("NTGCALLS", instance.createCall(CHAT_ID));
        } catch (FileNotFoundException e) {
            fail("Unexpected FileNotFoundException");
        } catch (ConnectionException e) {
            fail("Unexpected ConnectionException");
        }
    }

    @Test
    public void connectCall() {
        var instance = new NTgCalls();
        try {
            instance.createCall(CHAT_ID);
        } catch (FileNotFoundException e) {
            fail("Unexpected FileNotFoundException");
        } catch (ConnectionException e) {
            fail("Unexpected ConnectionException");
        }
        try {
            instance.connect(CHAT_ID, "{}", false);
        } catch (InvalidParamsException ignored) {
        } catch (TelegramServerException e) {
            fail("Unexpected TelegramServerException");
        } catch (ConnectionException e) {
            fail("Unexpected ConnectionException");
        }
    }

    @Test
    public void createAudioDevice() {
        //MTProtoTelegramClient client = MTProtoTelegramClient.create(
        NTgCalls instance = new NTgCalls();
        MediaDevices mediaDevices = NTgCalls.getMediaDevices();
        try {
            instance.createP2PCall(
                    CHAT_ID,
                    new MediaDescription(
                            new AudioDescription(
                                    MediaSource.DEVICE,
                                    mediaDevices.audio.get(0).metadata,
                                    48000,
                                    16,
                                    2
                            ),
                            null,
                            null,
                            null
                    )
            );
        } catch (FileNotFoundException e) {
            //throw new RuntimeException(e);
        } catch (ConnectionException e) {
            //throw new RuntimeException(e);
        }
    }
}
