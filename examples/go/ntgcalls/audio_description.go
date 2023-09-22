package ntgcalls

//#include "ntgcalls.h"
//#include <stdlib.h>
import "C"

type AudioDescription struct {
	InputMode                   InputMode
	Input                       string
	SampleRate                  uint32
	BitsPerSample, ChannelCount uint8
}

func (ctx *AudioDescription) ParseToC() C.ntg_audio_description_struct {
	var x C.ntg_audio_description_struct
	x.inputMode = ctx.InputMode.ParseToC()
	x.input = C.CString(ctx.Input)
	x.sampleRate = C.uint32_t(ctx.SampleRate)
	x.bitsPerSample = C.uint8_t(ctx.BitsPerSample)
	x.channelCount = C.uint8_t(ctx.ChannelCount)
	return x
}
