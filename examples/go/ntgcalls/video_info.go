package ntgcalls

type VideoInfo struct {
	Codec       string
	Width       uint16
	Height      uint16
	FrameRate   float64
	BFrames     bool
	Supported   bool
	Passthrough bool
}
