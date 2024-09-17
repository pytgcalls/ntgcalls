package org.pytgcalls.ntgcalls.exceptions;

public class TelegramServerException extends ConnectionException {
    public TelegramServerException(String message) {
        super(message);
    }
}
