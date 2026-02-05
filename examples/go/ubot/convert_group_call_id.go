package ubot

import (
	"fmt"

	tg "github.com/amarnathcjd/gogram/telegram"
)

func (ctx *Context) convertGroupCallId(callId int64) (int64, error) {
	for chatId, inputCallInterface := range ctx.inputGroupCalls {
		if inputCall, ok := inputCallInterface.(*tg.InputGroupCallObj); ok {
			if inputCall.ID == callId {
				return chatId, nil
			}
		}
	}
	return 0, fmt.Errorf("group call id %d not found", callId)
}
