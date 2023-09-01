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

func (ctx *VideoDescription) ParseToC() C.VideoDescription {
	var x C.VideoDescription
	x.inputMode = ctx.InputMode.ParseToC()
	x.input = C.CString(ctx.Input)
	x.width = C.uint16_t(ctx.Width)
	x.height = C.uint16_t(ctx.Height)
	x.fps = C.uint8_t(ctx.Fps)
	return x
}
