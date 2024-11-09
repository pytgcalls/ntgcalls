package ntgcalls

//#include "ntgcalls.h"
import "C"

type StreamType int
type ConnectionKind int
type ConnectionState int
type StreamStatus int
type StreamMode int
type StreamDevice int
type RemoteSourceState int
type MediaSource int

type StreamEndCallback func(chatId int64, streamType StreamType, streamDevice StreamDevice)
type UpgradeCallback func(chatId int64, state MediaState)
type ConnectionChangeCallback func(chatId int64, state CallNetworkState)
type SignalCallback func(chatId int64, signal []byte)
type FrameCallback func(chatId int64, sourceId int64, mode StreamMode, device StreamDevice, data []byte, frameData FrameData)
type RemoteSourceCallback func(chatId int64, source RemoteSource)

const (
	MicrophoneStream StreamDevice = iota
	SpeakerStream
	CameraStream
	ScreenStream
)

const (
	AudioStream StreamType = iota
	VideoStream
)

const (
	MediaSourceFile MediaSource = 1 << iota
	MediaSourceShell
	MediaSourceFFmpeg
	MediaSourceDevice
	MediaSourceDesktop
	MediaSourceExternal
)

const (
	ActiveStream StreamStatus = iota
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

const (
	NormalConnection ConnectionKind = iota
	PresentationConnection
)

const (
	CaptureStream StreamMode = iota
	PlaybackStream
)

const (
	ActiveRemoteSource RemoteSourceState = iota
	SuspendedRemoteSource
	InactiveRemoteSource
)

func (ctx MediaSource) ParseToC() C.ntg_media_source_enum {
	switch ctx {
	case MediaSourceFile:
		return C.NTG_FILE
	case MediaSourceShell:
		return C.NTG_SHELL
	case MediaSourceFFmpeg:
		return C.NTG_FFMPEG
	case MediaSourceDevice:
		return C.NTG_DEVICE
	case MediaSourceDesktop:
		return C.NTG_DESKTOP
	case MediaSourceExternal:
		return C.NTG_EXTERNAL
	default:
		return C.NTG_FILE
	}
}

func (ctx StreamMode) ParseToC() C.ntg_stream_mode_enum {
	switch ctx {
	case CaptureStream:
		return C.NTG_STREAM_CAPTURE
	case PlaybackStream:
		return C.NTG_STREAM_PLAYBACK
	default:
		return C.NTG_STREAM_CAPTURE
	}
}

func (ctx StreamDevice) ParseToC() C.ntg_stream_device_enum {
	switch ctx {
	case MicrophoneStream:
		return C.NTG_STREAM_MICROPHONE
	case SpeakerStream:
		return C.NTG_STREAM_SPEAKER
	case CameraStream:
		return C.NTG_STREAM_CAMERA
	case ScreenStream:
		return C.NTG_STREAM_SCREEN
	default:
		return C.NTG_STREAM_MICROPHONE
	}
}
