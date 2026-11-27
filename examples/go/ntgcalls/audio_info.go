package ntgcalls

type AudioInfo struct {
	Codec       string
	SampleRate  uint32
	Channels    uint8
	Supported   bool
	Passthrough bool
}
