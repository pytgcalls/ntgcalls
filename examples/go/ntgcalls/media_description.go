package ntgcalls

//#include "ntgcalls.h"
import "C"

type MediaDescription struct {
	Microphone *AudioDescription
	Speaker    *AudioDescription
	Camera     *VideoDescription
	Screen     *VideoDescription
}

func (ctx *MediaDescription) ParseToC() C.ntg_media_description_struct {
	var x C.ntg_media_description_struct
	if ctx.Microphone != nil {
		microphone := ctx.Microphone.ParseToC()
		x.microphone = &microphone
	}
	if ctx.Speaker != nil {
		speaker := ctx.Speaker.ParseToC()
		x.speaker = &speaker
	}
	if ctx.Camera != nil {
		camera := ctx.Camera.ParseToC()
		x.camera = &camera
	}
	if ctx.Screen != nil {
		screen := ctx.Screen.ParseToC()
		x.screen = &screen
	}
	return x
}
