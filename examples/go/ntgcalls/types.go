package ntgcalls

//#include "ntgcalls.h"
import "C"

type StreamType int
type ConnectionMode int
type ConnectionKind int
type ConnectionState int
type StreamStatus int
type StreamMode int
type StreamDevice int
type MediaSource int
type Format int
type Codec int
type MediaSegmentQuality int
type MediaSegmentStatus int
type CallType int

type StreamEndCallback func(chatId int64, streamType StreamType, streamDevice StreamDevice)
type UpgradeCallback func(chatId int64, state MediaState)
type ConnectionChangeCallback func(chatId int64, state NetworkInfo)
type SignalCallback func(chatId int64, signal []byte)
type FrameCallback func(chatId int64, mode StreamMode, device StreamDevice, frames []Frame)
type RemoteSourceCallback func(chatId int64, source RemoteSource)
type BroadcastTimestampCallback func(chatId int64)

type BroadcastPartCallback func(chatId int64, segmentPartRequest SegmentPartRequest)

type EmojisCallback func(chatId int64, emojis string)
type RequestParticipantsCallback func(chatId int64, request ParticipantsRequest)
type OutboundBlockCallback func(chatId int64, block []byte)
type SubchainRequestCallback func(chatId int64, subchainRequest SubchainRequest)

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
	MediaSourceDevice
	MediaSourceDesktop
	MediaSourceExternal
)

const (
	FormatPCM Format = iota
	FormatOgg
	FormatI420
	FormatMatroska
)

const (
	CodecPCM Codec = iota
	CodecOpus
	CodecI420
	CodecH264
	CodecVP8
	CodecVP9
	CodecAV1
)

const (
	ActiveStream StreamStatus = iota
	PausedStream
	IdlingStream
)

const (
	RtcConnection ConnectionMode = iota
	StreamConnection
	RTMPConnection
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
	SegmentQualityNone MediaSegmentQuality = iota - 1
	SegmentQualityThumbnail
	SegmentQualityMedium
	SegmentQualityFull
)

const (
	SegmentStatusNotReady MediaSegmentStatus = iota
	SegmentStatusResyncNeeded
	SegmentStatusSuccess
)

const (
	GroupCallType CallType = iota
	P2PCallType
	ConferenceCallType
)

func (ctx MediaSource) ParseToC() C.ntg_media_source {
	switch ctx {
	case MediaSourceFile:
		return C.NTG_MEDIA_SOURCE_FILE
	case MediaSourceShell:
		return C.NTG_MEDIA_SOURCE_SHELL
	case MediaSourceDevice:
		return C.NTG_MEDIA_SOURCE_DEVICE
	case MediaSourceDesktop:
		return C.NTG_MEDIA_SOURCE_DESKTOP
	case MediaSourceExternal:
		return C.NTG_MEDIA_SOURCE_EXTERNAL
	default:
		return C.NTG_MEDIA_SOURCE_FILE
	}
}

func (ctx Format) ParseToC() C.ntg_format {
	switch ctx {
	case FormatOgg:
		return C.NTG_FORMAT_OGG
	case FormatI420:
		return C.NTG_FORMAT_I420
	case FormatMatroska:
		return C.NTG_FORMAT_MATROSKA
	default:
		return C.NTG_FORMAT_PCM
	}
}

func (ctx Codec) ParseToC() C.ntg_codec {
	switch ctx {
	case CodecOpus:
		return C.NTG_CODEC_OPUS
	case CodecI420:
		return C.NTG_CODEC_I420
	case CodecH264:
		return C.NTG_CODEC_H264
	case CodecVP8:
		return C.NTG_CODEC_VP8
	case CodecVP9:
		return C.NTG_CODEC_VP9
	case CodecAV1:
		return C.NTG_CODEC_AV1
	default:
		return C.NTG_CODEC_PCM
	}
}

func parseCodec(codec C.ntg_codec) Codec {
	switch codec {
	case C.NTG_CODEC_OPUS:
		return CodecOpus
	case C.NTG_CODEC_I420:
		return CodecI420
	case C.NTG_CODEC_H264:
		return CodecH264
	case C.NTG_CODEC_VP8:
		return CodecVP8
	case C.NTG_CODEC_VP9:
		return CodecVP9
	case C.NTG_CODEC_AV1:
		return CodecAV1
	default:
		return CodecPCM
	}
}

func (ctx StreamMode) ParseToC() C.ntg_stream_mode {
	switch ctx {
	case CaptureStream:
		return C.NTG_STREAM_MODE_CAPTURE
	case PlaybackStream:
		return C.NTG_STREAM_MODE_PLAYBACK
	default:
		return C.NTG_STREAM_MODE_CAPTURE
	}
}

func (ctx StreamDevice) ParseToC() C.ntg_stream_device {
	switch ctx {
	case MicrophoneStream:
		return C.NTG_STREAM_DEVICE_MICROPHONE
	case SpeakerStream:
		return C.NTG_STREAM_DEVICE_SPEAKER
	case CameraStream:
		return C.NTG_STREAM_DEVICE_CAMERA
	case ScreenStream:
		return C.NTG_STREAM_DEVICE_SCREEN
	default:
		return C.NTG_STREAM_DEVICE_MICROPHONE
	}
}

func (ctx MediaSegmentStatus) ParseToC() C.ntg_media_segment_part_status {
	switch ctx {
	case SegmentStatusNotReady:
		return C.NTG_MEDIA_SEGMENT_PART_STATUS_NOT_READY
	case SegmentStatusResyncNeeded:
		return C.NTG_MEDIA_SEGMENT_PART_STATUS_RESYNC_NEEDED
	case SegmentStatusSuccess:
		return C.NTG_MEDIA_SEGMENT_PART_STATUS_SUCCESS
	default:
		return C.NTG_MEDIA_SEGMENT_PART_STATUS_NOT_READY
	}
}
