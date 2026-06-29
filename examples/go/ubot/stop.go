package ubot

import "github.com/mtgo-labs/mtgo/tg"

func (ctx *Context) Stop(chatId any) error {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return err
	}
	ctx.presentations = stdRemove(ctx.presentations, parsedChatId)
	delete(ctx.callSources, parsedChatId)
	err = ctx.binding.Stop(parsedChatId)
	if err != nil {
		return err
	}
	_, err = ctx.invoke(
		&tg.PhoneLeaveGroupCallRequest{
			Call: ctx.inputGroupCalls[parsedChatId],
		},
	)
	if err != nil {
		return err
	}
	return nil
}
