package ntgcalls

type StreamType int
type ConnectionMode int
type ConnectionKind int
type ConnectionState int
type StreamStatus int
type StreamMode int
type StreamDevice int
type MediaSource int
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
type RequestParticipantsCallback func(chatId int64)
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
