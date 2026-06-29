package ubot

import (
	"context"
	"fmt"

	"github.com/mtgo-labs/mtgo/tg"
)

func (ctx *Context) getInputGroupCall(chatId int64) (tg.InputGroupCallClass, error) {
	if call, ok := ctx.inputGroupCalls[chatId]; ok {
		if call == nil {
			return nil, fmt.Errorf("group call for chatId %d is closed", chatId)
		}
		return call, nil
	}
	peer, err := ctx.app.ResolvePeer(context.Background(), chatId)
	if err != nil {
		return nil, err
	}
	switch chatPeer := peer.(type) {
	case *tg.InputPeerChannel:
		fullChat, err := ctx.invoke(
			&tg.ChannelsGetFullChannelRequest{
				Channel: &tg.InputChannel{
					ChannelID:  chatPeer.ChannelID,
					AccessHash: chatPeer.AccessHash,
				},
			},
		)
		if err != nil {
			return nil, err
		}
		ctx.inputGroupCalls[chatId] = fullChat.(*tg.MessagesChatFull).FullChat.(*tg.ChannelFull).Call
	case *tg.InputPeerChat:
		fullChat, err := ctx.invoke(
			&tg.MessagesGetFullChatRequest{
				ChatID: chatPeer.ChatID,
			},
		)
		if err != nil {
			return nil, err
		}
		ctx.inputGroupCalls[chatId] = fullChat.(*tg.MessagesChatFull).FullChat.(*tg.ChatFull).Call
	default:
		return nil, fmt.Errorf("chatId %d is not a group call", chatId)
	}
	if call, ok := ctx.inputGroupCalls[chatId]; ok && call == nil {
		return nil, fmt.Errorf("group call for chatId %d is closed", chatId)
	} else if ok {
		return call, nil
	} else {
		return nil, fmt.Errorf("group call for chatId %d not found", chatId)
	}
}
