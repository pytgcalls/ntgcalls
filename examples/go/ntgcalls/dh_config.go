package ntgcalls

//#include "ntgcalls.h"
import "C"

type DhConfig struct {
	G      int32
	P      []byte
	Random []byte
}

func (ctx *DhConfig) ParseToC() C.ntg_dh_config_struct {
	var x C.ntg_dh_config_struct
	x.g = C.int32_t(ctx.G)
	pC, pSize := parseBytes(ctx.P)
	rC, rSize := parseBytes(ctx.Random)
	x.p = pC
	x.sizeP = pSize
	x.random = rC
	x.sizeRandom = rSize
	return x
}
