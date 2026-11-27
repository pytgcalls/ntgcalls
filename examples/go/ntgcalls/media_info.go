package ntgcalls

type MediaInfo struct {
	Container  string
	DurationMs int64
	Audio      *AudioInfo
	Video      *VideoInfo
}
