package ntgcalls

//#include "ntgcalls.h"
import "C"

type StreamType int
type StreamStatus int
type InputMode int

type StreamEndCallback func(chatId int64, streamType StreamType)
type UpgradeCallback func(chatId int64, streamType MediaState)

const (
	AudioStream StreamType = iota
	VideoStream
)

const (
	InputModeFile InputMode = iota
	InputModeShell
	InputModeFFmpeg
)

const (
	PlayingStream StreamStatus = iota
	PausedStream
	IdlingStream
)

func (ctx InputMode) ParseToC() C.ntg_input_mode_enum {
	switch ctx {
	case InputModeFile:
		return C.NTG_FILE
	case InputModeShell:
		return C.NTG_SHELL
	case InputModeFFmpeg:
		return C.NTG_FFMPEG
	}
	return C.NTG_FILE
}
