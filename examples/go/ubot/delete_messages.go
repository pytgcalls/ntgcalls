package ubot

import (
	"context"
	"fmt"

	"github.com/mtgo-labs/mtgo/tg"
)

func (ctx *Context) DeleteMessages(chatId any, messageIds []int32, reportSpam bool) error {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return err
	}
	if ctx.isConference(parsedChatId) {
		return fmt.Errorf("deleting messages is not supported in conference calls")
	}
	inputCall, err := ctx.getInputGroupCall(parsedChatId)
	if err != nil {
		return err
	}
	_, err = ctx.invoke(
		&tg.PhoneDeleteGroupCallMessagesRequest{
			ReportSpam: reportSpam,
			Call:       inputCall,
			Messages:   messageIds,
		},
	)
	return err
}

func (ctx *Context) DeleteParticipantMessages(chatId any, participantId any, reportSpam bool) error {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return err
	}
	if ctx.isConference(parsedChatId) {
		return fmt.Errorf("deleting messages is not supported in conference calls")
	}
	inputCall, err := ctx.getInputGroupCall(parsedChatId)
	if err != nil {
		return err
	}
	parsedParticipantId, err := ctx.parseChatId(participantId)
	if err != nil {
		return err
	}
	participant, err := ctx.app.ResolvePeer(context.Background(), parsedParticipantId)
	if err != nil {
		return err
	}
	_, err = ctx.invoke(
		&tg.PhoneDeleteGroupCallParticipantMessagesRequest{
			ReportSpam:  reportSpam,
			Call:        inputCall,
			Participant: participant,
		},
	)
	return err
}
