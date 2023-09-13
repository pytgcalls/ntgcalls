package ntgcalls

//#include "ntgcalls.h"
//#include <stdlib.h>
import "C"

type VideoDescription struct {
	InputMode     InputMode
	Input         string
	Width, Height uint16
	Fps           uint8
}

func (ctx *VideoDescription) ParseToC() C.ntg_video_description_struct {
	var x C.ntg_video_description_struct
	x.inputMode = ctx.InputMode.ParseToC()
	x.input = C.CString(ctx.Input)
	x.width = C.uint16_t(ctx.Width)
	x.height = C.uint16_t(ctx.Height)
	x.fps = C.uint8_t(ctx.Fps)
	return x
}
