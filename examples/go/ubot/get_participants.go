package ubot

import (
	"gotgcalls/ubot/types"
	"maps"
	"slices"
	"time"

	"github.com/mtgo-labs/mtgo/tg"
)

func (ctx *Context) GetParticipants(chatId int64) ([]*tg.GroupCallParticipant, error) {
	ctx.participantsMutex.Lock()
	defer ctx.participantsMutex.Unlock()
	if ctx.callParticipants[chatId] == nil {
		ctx.callParticipants[chatId] = &types.CallParticipantsCache{
			CallParticipants: make(map[int64]*tg.GroupCallParticipant),
		}
	}
	if time.Since(ctx.callParticipants[chatId].LastMTProtoUpdate) > time.Minute {
		groupCall, err := ctx.getInputGroupCall(chatId)
		if err != nil {
			return nil, err
		}
		ctx.callParticipants[chatId].CallParticipants = make(map[int64]*tg.GroupCallParticipant)
		var nextOffset string
		for {
			resRaw, err := ctx.invoke(
				&tg.PhoneGetGroupParticipantsRequest{
					Call:   groupCall,
					Offset: nextOffset,
				},
			)
			if err != nil {
				return nil, err
			}
			res := resRaw.(*tg.PhoneGroupParticipants)
			for _, participant := range res.Participants {
				ctx.callParticipants[chatId].CallParticipants[parsePeer(participant.Peer)] = participant
			}
			if res.NextOffset == "" {
				break
			}
			nextOffset = res.NextOffset
		}
		ctx.callParticipants[chatId].LastMTProtoUpdate = time.Now()
	}
	return slices.Collect(maps.Values(ctx.callParticipants[chatId].CallParticipants)), nil
}
