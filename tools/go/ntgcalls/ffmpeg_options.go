package ntgcalls

//#include "ntgcalls.h"
import "C"

type FFmpegOptions struct {
	StreamId uint8
}

func (ctx *FFmpegOptions) ParseToC() C.FFmpegOptions {
	var x C.FFmpegOptions
	x.streamId = C.uint8_t(ctx.StreamId)
	return x
}
