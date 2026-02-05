package ntgcalls

//#include "ntgcalls.h"
//#include <stdlib.h>
import "C"

type VideoDescription struct {
	MediaSource   MediaSource
	Input         string
	Width, Height int16
	Fps           uint8
	KeepOpen      bool
}

func (ctx *VideoDescription) ParseToC() C.ntg_video_description_struct {
	var x C.ntg_video_description_struct
	x.mediaSource = ctx.MediaSource.ParseToC()
	x.input = C.CString(ctx.Input)
	x.width = C.int16_t(ctx.Width)
	x.height = C.int16_t(ctx.Height)
	x.fps = C.uint8_t(ctx.Fps)
	x.keepOpen = C.bool(ctx.KeepOpen)
	return x
}
