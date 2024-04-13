package ntgcalls

//#include "ntgcalls.h"
import "C"

type DhConfig struct {
	g      int32
	p      []byte
	random []byte
}

func (ctx *DhConfig) ParseToC() C.ntg_dh_config_struct {
	var x C.ntg_dh_config_struct
	x.g = C.int32_t(ctx.g)
	pC, pSize := parseBytes(ctx.p)
	rC, rSize := parseBytes(ctx.random)
	x.p = pC
	x.sizeP = pSize
	x.random = rC
	x.sizeRandom = rSize
	return x
}
