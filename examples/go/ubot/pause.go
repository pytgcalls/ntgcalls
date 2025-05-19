package ubot

func (ctx *Context) Pause(chatId any) (bool, error) {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return false, err
	}
	return ctx.binding.Pause(parsedChatId)
}
