package ntgcalls

//#include "ntgcalls.h"
import "C"

type FrameData struct {
	AbsoluteCaptureTimestampMs int64
	Width, Height, Rotation    uint16
}

func (ctx *FrameData) ParseToC() C.ntg_frame_data_struct {
	var x C.ntg_frame_data_struct
	x.absoluteCaptureTimestampMs = C.int64_t(ctx.AbsoluteCaptureTimestampMs)
	x.width = C.uint16_t(ctx.Width)
	x.height = C.uint16_t(ctx.Height)
	x.rotation = C.uint16_t(ctx.Rotation)
	return x
}
