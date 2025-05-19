package ubot

func (ctx *Context) Resume(chatId any) (bool, error) {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return false, err
	}
	return ctx.binding.Resume(parsedChatId)
}
