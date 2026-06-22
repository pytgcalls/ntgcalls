package ubot

import (
	"gotgcalls/ntgcalls"

	tg "github.com/amarnathcjd/gogram/telegram"
)

func (ctx *Context) getConferenceLastBlock(chatId int64) ([]byte, error) {
	inputCall, err := ctx.getInputGroupCall(chatId)
	if err != nil {
		return nil, err
	}
	res, err := ctx.app.PhoneGetGroupCallChainBlocks(inputCall, 0, -1, 1)
	if err != nil {
		return nil, err
	}
	if updatesObj, ok := res.(*tg.UpdatesObj); ok {
		for _, update := range updatesObj.Updates {
			if chainBlocks, ok := update.(*tg.UpdateGroupCallChainBlocks); ok {
				if len(chainBlocks.Blocks) > 0 {
					return chainBlocks.Blocks[len(chainBlocks.Blocks)-1], nil
				}
			}
		}
	}
	return nil, nil
}

func (ctx *Context) sendConferenceCallBroadcast(chatId int64, block []byte) {
	inputCall, err := ctx.getInputGroupCall(chatId)
	if err != nil {
		return
	}
	_, _ = ctx.app.PhoneSendConferenceCallBroadcast(inputCall, block)
}

func (ctx *Context) getSubchainBlocks(chatId int64, request ntgcalls.SubchainRequest) ([][]byte, int32, error) {
	inputCall, err := ctx.getInputGroupCall(chatId)
	if err != nil {
		return nil, 0, err
	}
	res, err := ctx.app.PhoneGetGroupCallChainBlocks(inputCall, request.Subchain, request.Height, request.Limit)
	if err != nil {
		return nil, 0, err
	}
	if updatesObj, ok := res.(*tg.UpdatesObj); ok {
		for _, update := range updatesObj.Updates {
			if chainBlocks, ok := update.(*tg.UpdateGroupCallChainBlocks); ok {
				return chainBlocks.Blocks, chainBlocks.NextOffset, nil
			}
		}
	}
	return nil, 0, nil
}

func (ctx *Context) JoinConference(chatId any, mediaDescription ntgcalls.MediaDescription) error {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return err
	}
	return ctx.connectCall(parsedChatId, mediaDescription, "", true, nil)
}
