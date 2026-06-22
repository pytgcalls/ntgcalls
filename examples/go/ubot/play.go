package ubot

import (
	"gotgcalls/ntgcalls"
)

func (ctx *Context) Play(chatId any, mediaDescription ntgcalls.MediaDescription) error {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return err
	}
	if ctx.binding.Calls()[parsedChatId] != nil {
		return ctx.binding.SetStreamSources(parsedChatId, ntgcalls.CaptureStream, mediaDescription)
	}
	return ctx.connectCall(parsedChatId, mediaDescription, "", false, nil)
}
