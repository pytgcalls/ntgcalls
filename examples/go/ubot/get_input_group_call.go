package ubot

import (
	"fmt"
	tg "github.com/amarnathcjd/gogram/telegram"
)

func (ctx *Context) getInputGroupCall(chatId int64) (*tg.InputGroupCall, error) {
	if ctx.inputGroupCalls[chatId] != nil {
		return ctx.inputGroupCalls[chatId], nil
	}
	peer, err := ctx.app.ResolvePeer(chatId)
	if err != nil {
		return nil, err
	}
	switch chatPeer := peer.(type) {
	case *tg.InputPeerChannel:
		fullChat, err := ctx.app.ChannelsGetFullChannel(
			&tg.InputChannelObj{
				ChannelID:  chatPeer.ChannelID,
				AccessHash: chatPeer.AccessHash,
			},
		)
		if err != nil {
			return nil, err
		}
		ctx.inputGroupCalls[chatId] = fullChat.FullChat.(*tg.ChannelFull).Call
	case *tg.InputPeerChat:
		fullChat, err := ctx.app.MessagesGetFullChat(chatPeer.ChatID)
		if err != nil {
			return nil, err
		}
		ctx.inputGroupCalls[chatId] = fullChat.FullChat.(*tg.ChatFullObj).Call
	default:
		return nil, fmt.Errorf("chatId %d is not a group call", chatId)
	}
	return ctx.inputGroupCalls[chatId], nil
}
