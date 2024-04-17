package ntgcalls

//#include "ntgcalls.h"
import "C"

type StreamType int
type ConnectionState int
type StreamStatus int
type InputMode int

type StreamEndCallback func(chatId int64, streamType StreamType)
type UpgradeCallback func(chatId int64, state MediaState)
type ConnectionChangeCallback func(chatId int64, state ConnectionState)
type SignalCallback func(chatId int64, signal []byte)

const (
	AudioStream StreamType = iota
	VideoStream
)

const (
	InputModeFile InputMode = 1 << iota
	InputModeShell
	InputModeFFmpeg
	InputModeNoLatency
)

const (
	PlayingStream StreamStatus = iota
	PausedStream
	IdlingStream
)

const (
	Connecting ConnectionState = iota
	Connected
	Failed
	Timeout
	Closed
)

func (ctx InputMode) ParseToC() C.ntg_input_mode_enum {
	switch ctx {
	case InputModeFile:
		return C.NTG_FILE
	case InputModeShell:
		return C.NTG_SHELL
	case InputModeFFmpeg:
		return C.NTG_FFMPEG
	default:
		return C.NTG_FILE
	}
}
