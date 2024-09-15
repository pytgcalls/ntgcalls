import android.util.Log;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.pytgcalls.ntgcalls.NTgCalls;
import org.pytgcalls.ntgcalls.media.AudioDescription;
import org.pytgcalls.ntgcalls.media.InputMode;
import org.pytgcalls.ntgcalls.media.MediaDescription;

@RunWith(AndroidJUnit4.class)
public class NTgCallsTests {
    @Test
    public void ping() {
        Log.d("NTGCALLS", String.format("Ping Time: %d", NTgCalls.ping()));
    }

    @Test
    public void classInstance() {
        var tmpClass = new NTgCalls();
        var mediaDescription = new MediaDescription(
            new AudioDescription(
                    InputMode.FFMPEG,
                    "input",
                    48000,
                    16,
                    2
            )
        );
        tmpClass.createCall(1234567890, mediaDescription);
        Log.d("NTGCALLS", tmpClass.toString());
    }
}
