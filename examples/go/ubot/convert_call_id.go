package ubot

import "fmt"

func (ctx *Context) convertCallId(callId int64) (int64, error) {
	for chatId, inputCall := range ctx.inputCalls {
		if inputCall.ID == callId {
			return chatId, nil
		}
	}
	return 0, fmt.Errorf("call id %d not found", callId)
}
