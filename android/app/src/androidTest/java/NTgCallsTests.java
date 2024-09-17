import android.util.Log;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import static org.junit.Assert.fail;
import org.junit.runner.RunWith;
import org.pytgcalls.ntgcalls.NTgCalls;
import org.pytgcalls.ntgcalls.exceptions.ConnectionException;
import org.pytgcalls.ntgcalls.exceptions.InvalidParamsException;
import org.pytgcalls.ntgcalls.exceptions.TelegramServerException;
import org.pytgcalls.ntgcalls.media.AudioDescription;
import org.pytgcalls.ntgcalls.media.InputMode;
import org.pytgcalls.ntgcalls.media.MediaDescription;

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
                                    InputMode.FILE,
                                    "input",
                                    48000,
                                    16,
                                    2
                            )
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
            instance.connect(CHAT_ID, "{}");
        } catch (InvalidParamsException ignored) {
        } catch (TelegramServerException e) {
            fail("Unexpected TelegramServerException");
        } catch (ConnectionException e) {
            fail("Unexpected ConnectionException");
        }
    }
}
