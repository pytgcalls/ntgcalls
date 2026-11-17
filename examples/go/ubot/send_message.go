package ubot

import (
	"crypto/rand"
	"encoding/binary"
	"gotgcalls/ubot/types"
	"time"

	"github.com/mtgo-labs/mtgo/tg"
)

func randomId() int64 {
	var buffer [8]byte
	_, _ = rand.Read(buffer[:])
	return int64(binary.LittleEndian.Uint64(buffer[:]))
}

func (ctx *Context) SendMessage(chatId any, text string, stars int64) (*types.CallMessage, error) {
	parsedChatId, err := ctx.parseChatId(chatId)
	if err != nil {
		return nil, err
	}
	inputCall, err := ctx.getInputGroupCall(parsedChatId)
	if err != nil {
		return nil, err
	}
	if text == "" && stars == 0 {
		return nil, nil
	}
	message, entities := parseHTML(text)
	messageRandomId := randomId()
	if ctx.isConference(parsedChatId) {
		return ctx.sendConferenceMessage(parsedChatId, inputCall, messageRandomId, message, conferenceEntities(entities))
	}
	ctx.messagesMutex.Lock()
	ctx.getCallMessages(parsedChatId).SendingIDs[messageRandomId] = struct{}{}
	ctx.messagesMutex.Unlock()
	result, err := ctx.invoke(
		&tg.PhoneSendGroupCallMessageRequest{
			Call:     inputCall,
			RandomID: messageRandomId,
			Message: &tg.TextWithEntities{
				Text:     message,
				Entities: entities,
			},
			AllowPaidStars: stars,
			SendAs:         ctx.self,
		},
	)
	if err != nil {
		ctx.messagesMutex.Lock()
		delete(ctx.getCallMessages(parsedChatId).SendingIDs, messageRandomId)
		ctx.messagesMutex.Unlock()
		return nil, err
	}
	var sent *types.CallMessage
	if updates, ok := result.(*tg.Updates); ok {
		for _, update := range updates.Updates {
			if u, ok := update.(*tg.UpdateGroupCallMessage); ok {
				sent = parseCallMessage(u.Message)
				ctx.callMessageSent(messageRandomId, sent.MessageID)
			}
		}
	}
	return sent, nil
}

func (ctx *Context) sendConferenceMessage(chatId int64, inputCall tg.InputGroupCallClass, messageRandomId int64, message string, entities []tg.MessageEntityClass) (*types.CallMessage, error) {
	serialized, err := serializeConferenceMessage(messageRandomId, message, entities)
	if err != nil {
		return nil, err
	}
	encrypted, err := ctx.binding.EncryptMessage(chatId, serialized)
	if err != nil {
		return nil, err
	}
	messageId := ctx.conferenceMessageSent(chatId, messageRandomId)
	if _, err = ctx.invoke(
		&tg.PhoneSendGroupCallEncryptedMessageRequest{
			Call:             inputCall,
			EncryptedMessage: encrypted,
		},
	); err != nil {
		return nil, err
	}
	return &types.CallMessage{
		MessageID: messageId,
		SenderID:  ctx.self.UserID,
		Text:      message,
		HTML:      unparseHTML(message, entities),
		Date:      int32(time.Now().Unix()),
	}, nil
}
