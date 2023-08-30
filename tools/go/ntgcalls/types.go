package ntgcalls

type StreamType int

type StreamEndCallback func(chatId int64, streamType StreamType)
type UpgradeCallback func(chatId int64, streamType MediaState)

const (
	AudioStream StreamType = iota
	VideoStream
)
