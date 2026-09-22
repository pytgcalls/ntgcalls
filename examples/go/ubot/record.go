package ubot

import "gotgcalls/ntgcalls"

func (ctx *Context) Record(chatId any, mediaDescription ntgcalls.MediaDescription) error {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return err
	}
	calls, err := ctx.binding.Calls()
	if err != nil {
		return err
	}
	if _, ok := calls[parsedChatId]; !ok {
		if err = ctx.Play(chatId, ntgcalls.MediaDescription{}); err != nil {
			return err
		}
	}
	return ctx.binding.SetStreamSources(parsedChatId, ntgcalls.PlaybackStream, mediaDescription)
}
