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
	ctx.inputGroupCallsMutex.RLock()
	inputGroupCall := ctx.inputGroupCalls[parsedChatId]
	ctx.inputGroupCallsMutex.RUnlock()
	_, err = ctx.app.PhoneLeaveGroupCall(inputGroupCall, 0)
	if err != nil {
		return err
	}
	return nil
}
