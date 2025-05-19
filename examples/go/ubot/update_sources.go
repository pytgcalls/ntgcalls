package ubot

import (
	"gotgcalls/ubot/types"
	"slices"
)

func (ctx *Context) updateSources(chatId int64) error {
	participants, err := ctx.GetParticipants(chatId)
	if err != nil {
		return err
	}
	if ctx.callSources[chatId] == nil {
		ctx.callSources[chatId] = &types.CallSources{
			CameraSources: make(map[int64]string),
			ScreenSources: make(map[int64]string),
		}
	}
	for _, participant := range participants {
		participantId := getParticipantId(participant.Peer)
		if participant.Video != nil && ctx.callSources[chatId].CameraSources[participantId] == "" {
			ctx.callSources[chatId].CameraSources[participantId] = participant.Video.Endpoint
			_, err = ctx.binding.AddIncomingVideo(chatId, participant.Video.Endpoint, parseVideoSources(participant.Video.SourceGroups))
			if err != nil {
				return err
			}
		}
		if participant.Presentation != nil && ctx.callSources[chatId].ScreenSources[participantId] == "" {
			ctx.callSources[chatId].ScreenSources[participantId] = participant.Presentation.Endpoint
			_, err = ctx.binding.AddIncomingVideo(chatId, participant.Presentation.Endpoint, parseVideoSources(participant.Presentation.SourceGroups))
			if err != nil {
				return err
			}
		}
		if participantId == ctx.self.ID && !participant.CanSelfUnmute && !slices.Contains(ctx.mutedByAdmin, chatId) {
			ctx.mutedByAdmin = append(ctx.mutedByAdmin, chatId)
		}
	}
	return nil
}
