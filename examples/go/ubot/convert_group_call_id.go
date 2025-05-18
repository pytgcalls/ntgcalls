package ubot

import "fmt"

func (ctx *Context) convertGroupCallId(callId int64) (int64, error) {
	for chatId, inputCall := range ctx.inputGroupCalls {
		if inputCall.ID == callId {
			return chatId, nil
		}
	}
	return 0, fmt.Errorf("group call id %d not found", callId)
}
