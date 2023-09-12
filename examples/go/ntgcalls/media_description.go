package ntgcalls

//#include "ntgcalls.h"
import "C"

type MediaDescription struct {
	Audio *AudioDescription
	Video *VideoDescription
}

func (ctx *MediaDescription) ParseToC() C.ntg_media_description_struct {
	var x C.ntg_media_description_struct
	if ctx.Audio != nil {
		audio := ctx.Audio.ParseToC()
		x.audio = &audio
	}
	if ctx.Video != nil {
		video := ctx.Video.ParseToC()
		x.video = &video
	}
	return x
}
