package io.github.pytgcalls.exceptions;

import java.io.IOException;

public class ConnectionException extends IOException {
    public ConnectionException(String message) {
        super(message);
    }
}
