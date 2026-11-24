package ntgcalls

//#include "ntgcalls.h"
//#include <stdlib.h>
import "C"
import "unsafe"

type VideoDescription struct {
	MediaSource   MediaSource
	Input         string
	Width, Height int16
	Fps           uint8
	KeepOpen      bool
	Format        *Format
}

func (ctx *VideoDescription) ParseToC() (C.ntg_video_description, func()) {
	var x C.ntg_video_description
	x.media_source = ctx.MediaSource.ParseToC()
	x.input = C.CString(ctx.Input)
	x.width = C.int16_t(ctx.Width)
	x.height = C.int16_t(ctx.Height)
	x.fps = C.uint8_t(ctx.Fps)
	x.keep_open = C.bool(ctx.KeepOpen)
	if ctx.Format != nil {
		x.format = (*C.ntg_format)(C.malloc(C.size_t(unsafe.Sizeof(C.ntg_format(0)))))
		*x.format = ctx.Format.ParseToC()
	}
	return x, func() {
		C.free(unsafe.Pointer(x.input))
		if x.format != nil {
			C.free(unsafe.Pointer(x.format))
		}
	}
}
