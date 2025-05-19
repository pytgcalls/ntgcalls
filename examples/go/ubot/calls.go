package ubot

import "gotgcalls/ntgcalls"

func (ctx *Context) Calls() map[int64]*ntgcalls.CallInfo {
	return ctx.binding.Calls()
}
