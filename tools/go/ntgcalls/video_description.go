package ntgcalls

//#include "ntgcalls.h"
//#include <stdlib.h>
import "C"
import "unsafe"

type VideoDescription struct {
	Width, Height uint16
	Fps           uint8
	Path          string
	Options       *FFmpegOptions
}

func (ctx *VideoDescription) ParseToC() C.VideoDescription {
	var x C.VideoDescription
	x.width = C.uint16_t(ctx.Width)
	x.height = C.uint16_t(ctx.Height)
	x.fps = C.uint8_t(ctx.Fps)
	x.path = C.CString(ctx.Path)
	if ctx.Options != nil {
		options := ctx.Options.ParseToC()
		x.options = &options
	}
	defer C.free(unsafe.Pointer(x.path))
	return x
}
