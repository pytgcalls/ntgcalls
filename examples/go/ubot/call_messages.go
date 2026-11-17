package ubot

import (
	"gotgcalls/ntgcalls"
	"gotgcalls/ubot/types"
	"time"

	"github.com/Laky-64/gologging"
	"github.com/mtgo-labs/mtgo/tg"
)

func (ctx *Context) getCallMessages(chatId int64) *types.CallMessagesCache {
	if ctx.callMessages[chatId] == nil {
		ctx.callMessages[chatId] = &types.CallMessagesCache{
			SendingIDs:    make(map[int64]struct{}),
			MessageIDs:    make(map[int32]struct{}),
			ConferenceIDs: make(map[int64]int32),
		}
	}
	return ctx.callMessages[chatId]
}

func (ctx *Context) isConference(chatId int64) bool {
	callType, err := ctx.binding.GetCallType(chatId)
	return err == nil && callType == ntgcalls.ConferenceCallType
}

func (ctx *Context) emitCallMessage(chatId int64, action types.CallMessageAction, messageId int32, message *types.CallMessage) {
	for _, callback := range ctx.callMessageCallbacks {
		go callback(chatId, action, messageId, message)
	}
}

func (ctx *Context) callMessageSent(randomId int64, messageId int32) {
	ctx.messagesMutex.Lock()
	defer ctx.messagesMutex.Unlock()
	for _, cache := range ctx.callMessages {
		if _, ok := cache.SendingIDs[randomId]; ok {
			delete(cache.SendingIDs, randomId)
			cache.MessageIDs[messageId] = struct{}{}
			return
		}
	}
}

func (ctx *Context) conferenceMessageSent(chatId int64, randomId int64) int32 {
	ctx.messagesMutex.Lock()
	defer ctx.messagesMutex.Unlock()
	cache := ctx.getCallMessages(chatId)
	cache.ConferenceAutoIncrement++
	cache.ConferenceIDs[randomId] = cache.ConferenceAutoIncrement
	return cache.ConferenceAutoIncrement
}

func parseCallMessage(message *tg.GroupCallMessage) *types.CallMessage {
	return &types.CallMessage{
		MessageID: message.ID,
		SenderID:  parsePeer(message.FromID),
		Text:      message.Message.Text,
		HTML:      unparseHTML(message.Message.Text, message.Message.Entities),
		Date:      message.Date,
		Stars:     message.PaidMessageStars,
		FromAdmin: message.FromAdmin,
	}
}

func (ctx *Context) callMessageReceived(chatId int64, message *tg.GroupCallMessage) {
	if ctx.isConference(chatId) {
		return
	}
	if message.Message.Text == "" && message.PaidMessageStars == 0 {
		return
	}
	ctx.messagesMutex.Lock()
	cache := ctx.getCallMessages(chatId)
	if _, ok := cache.MessageIDs[message.ID]; ok {
		ctx.messagesMutex.Unlock()
		return
	}
	cache.MessageIDs[message.ID] = struct{}{}
	ctx.messagesMutex.Unlock()
	ctx.emitCallMessage(chatId, types.CallMessageReceived, message.ID, parseCallMessage(message))
}

func (ctx *Context) callMessagesDeleted(chatId int64, messageIds []int32) {
	ctx.messagesMutex.Lock()
	cache := ctx.getCallMessages(chatId)
	var deleted []int32
	for _, messageId := range messageIds {
		if _, ok := cache.MessageIDs[messageId]; ok {
			delete(cache.MessageIDs, messageId)
			deleted = append(deleted, messageId)
		}
	}
	ctx.messagesMutex.Unlock()
	for _, messageId := range deleted {
		ctx.emitCallMessage(chatId, types.CallMessageDeleted, messageId, nil)
	}
}

func (ctx *Context) encryptedCallMessageReceived(chatId int64, fromId tg.PeerClass, data []byte) {
	if !ctx.isConference(chatId) {
		return
	}
	userId := parsePeer(fromId)
	decrypted, err := ctx.binding.DecryptMessage(chatId, userId, data)
	if err != nil {
		gologging.DebugF("decrypt call message: %v", err)
		return
	}
	randomId, text, entities, ok := deserializeConferenceMessage(decrypted)
	if !ok {
		gologging.Debug("bad decrypted call message")
		return
	}
	ctx.messagesMutex.Lock()
	cache := ctx.getCallMessages(chatId)
	if _, exists := cache.ConferenceIDs[randomId]; exists {
		ctx.messagesMutex.Unlock()
		return
	}
	ctx.messagesMutex.Unlock()
	messageId := ctx.conferenceMessageSent(chatId, randomId)
	ctx.emitCallMessage(chatId, types.CallMessageReceived, messageId, &types.CallMessage{
		MessageID: messageId,
		SenderID:  userId,
		Text:      text,
		HTML:      unparseHTML(text, entities),
		Date:      int32(time.Now().Unix()),
	})
}

func (ctx *Context) handleCallMessageUpdate(update tg.UpdateClass) {
	switch u := update.(type) {
	case *tg.UpdateMessageID:
		ctx.callMessageSent(u.RandomID, u.ID)
	case *tg.UpdateGroupCallMessage:
		if inputCall, ok := u.Call.(*tg.InputGroupCall); ok {
			if chatId, err := ctx.convertGroupCallId(inputCall.ID); err == nil {
				ctx.callMessageReceived(chatId, u.Message)
			}
		}
	case *tg.UpdateDeleteGroupCallMessages:
		if inputCall, ok := u.Call.(*tg.InputGroupCall); ok {
			if chatId, err := ctx.convertGroupCallId(inputCall.ID); err == nil {
				ctx.callMessagesDeleted(chatId, u.Messages)
			}
		}
	case *tg.UpdateGroupCallEncryptedMessage:
		if inputCall, ok := u.Call.(*tg.InputGroupCall); ok {
			if chatId, err := ctx.convertGroupCallId(inputCall.ID); err == nil {
				ctx.encryptedCallMessageReceived(chatId, u.FromID, u.EncryptedMessage)
			}
		}
	}
}
