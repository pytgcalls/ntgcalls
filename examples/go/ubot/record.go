package ubot

import "gotgcalls/ntgcalls"

func (ctx *Context) Record(chatId any, mediaDescription ntgcalls.MediaDescription) error {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return err
	}
	if ctx.binding.Calls()[parsedChatId] == nil {
		err = ctx.Play(chatId, ntgcalls.MediaDescription{})
		if err != nil {
			return err
		}
	}
	return ctx.binding.SetStreamSources(parsedChatId, ntgcalls.PlaybackStream, mediaDescription)
}
