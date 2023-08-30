package ntgcalls

//#include "ntgcalls.h"
//#include <stdlib.h>
import "C"
import "unsafe"

type AudioDescription struct {
	SampleRate                  uint16
	BitsPerSample, ChannelCount uint8
	Path                        string
	Options                     *FFmpegOptions
}

func (ctx *AudioDescription) ParseToC() C.AudioDescription {
	var x C.AudioDescription
	x.sampleRate = C.uint16_t(ctx.SampleRate)
	x.bitsPerSample = C.uint8_t(ctx.BitsPerSample)
	x.channelCount = C.uint8_t(ctx.ChannelCount)
	x.path = C.CString(ctx.Path)
	if ctx.Options != nil {
		options := ctx.Options.ParseToC()
		x.options = &options
	}
	defer C.free(unsafe.Pointer(x.path))
	return x
}
