package ntgcalls

//#include "ntgcalls.h"
import "C"

type StreamType int
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

func (ctx InputMode) ParseToC() C.enum_InputMode {
	switch ctx {
	case InputModeFile:
		return C.File
	case InputModeShell:
		return C.Shell
	case InputModeFFmpeg:
		return C.FFmpeg
	}
	return C.File
}
