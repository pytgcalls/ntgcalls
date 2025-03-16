package io.github.pytgcalls.exceptions;

public class TelegramServerException extends ConnectionException {
    public TelegramServerException(String message) {
        super(message);
    }
}
