package ntgcalls

//#include "ntgcalls.h"
//#include <stdlib.h>
import "C"
import "unsafe"

type MediaDescription struct {
	Encoder string
	Audio   *AudioDescription
	Video   *VideoDescription
}

func (ctx *MediaDescription) ParseToC() C.MediaDescription {
	var x C.MediaDescription
	if ctx.Audio != nil {
		audio := ctx.Audio.ParseToC()
		x.audio = &audio
	}
	if ctx.Video != nil {
		video := ctx.Video.ParseToC()
		x.video = &video
	}
	x.encoder = C.CString(ctx.Encoder)
	defer C.free(unsafe.Pointer(x.encoder))
	return x
}
