package ubot

func (ctx *Context) Stop(chatId any) error {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return err
	}
	ctx.presentations = stdRemove(ctx.presentations, parsedChatId)
	delete(ctx.pendingPresentation, parsedChatId)
	delete(ctx.callSources, parsedChatId)
	err = ctx.binding.Stop(parsedChatId)
	if err != nil {
		return err
	}
	_, err = ctx.app.PhoneLeaveGroupCall(ctx.inputGroupCalls[parsedChatId], 0)
	if err != nil {
		return err
	}
	return nil
}
