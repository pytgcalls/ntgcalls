package ubot

import (
	"fmt"
	"gotgcalls/ntgcalls"
	"gotgcalls/ubot/types"
	"slices"
	"time"

	"github.com/Laky-64/gologging"
	"github.com/mtgo-labs/mtgo/tg"
	"github.com/mtgo-labs/mtgo/tgerr"
)

func (ctx *Context) handleUpdates() {
	ctx.app.OnRawUpdate(func(update *tg.UpdatePhoneCallSignalingData) {
		userId, err := ctx.convertCallId(update.PhoneCallID)
		if err != nil {
			gologging.Error(err)
			return
		}
		_ = ctx.binding.SendSignalingData(userId, update.Data)
	})

	ctx.app.OnRawUpdate(func(update *tg.UpdatePhoneCall) {
		phoneCall := update.PhoneCall
		var ID int64
		var AccessHash int64
		var userId int64

		switch call := phoneCall.(type) {
		case *tg.PhoneCallAccepted:
			userId = call.ParticipantID
			ID = call.ID
			AccessHash = call.AccessHash
		case *tg.PhoneCallWaiting:
			userId = call.ParticipantID
			ID = call.ID
			AccessHash = call.AccessHash
		case *tg.PhoneCallRequested:
			userId = call.AdminID
			ID = call.ID
			AccessHash = call.AccessHash
		case *tg.PhoneCall:
			userId = call.AdminID
		case *tg.PhoneCallDiscarded:
			userId, _ = ctx.convertCallId(call.ID)
		}

		switch phoneCall.(type) {
		case *tg.PhoneCallAccepted, *tg.PhoneCallRequested, *tg.PhoneCallWaiting:
			ctx.inputCalls[userId] = &tg.InputPhoneCall{
				ID:         ID,
				AccessHash: AccessHash,
			}
		}

		switch call := phoneCall.(type) {
		case *tg.PhoneCallAccepted:
			if ctx.p2pConfigs[userId] != nil {
				ctx.p2pConfigs[userId].GAorB = call.GB
				ctx.p2pConfigs[userId].WaitData <- nil
			}
		case *tg.PhoneCall:
			if ctx.p2pConfigs[userId] != nil {
				ctx.p2pConfigs[userId].GAorB = call.GAOrB
				ctx.p2pConfigs[userId].KeyFingerprint = call.KeyFingerprint
				ctx.p2pConfigs[userId].PhoneCall = call
				ctx.p2pConfigs[userId].WaitData <- nil
			}
		case *tg.PhoneCallDiscarded:
			if migrate, ok := call.Reason.(*tg.PhoneCallDiscardReasonMigrateConferenceCall); ok {
				ctx.inputGroupCalls[userId] = &tg.InputGroupCallSlug{Slug: migrate.Slug}
				delete(ctx.inputCalls, userId)
				go func() {
					lastBlock, err := ctx.getConferenceLastBlock(userId)
					if err != nil {
						gologging.ErrorF("migrate getConferenceLastBlock: %v", err)
						return
					}
					if err = ctx.connectCall(userId, ntgcalls.MediaDescription{}, "", true, lastBlock); err != nil {
						gologging.ErrorF("migrate connectCall: %v", err)
					}
				}()
				return
			}
			var reasonMessage string
			switch call.Reason.(type) {
			case *tg.PhoneCallDiscardReasonBusy:
				reasonMessage = fmt.Sprintf("the user %d is busy", userId)
			case *tg.PhoneCallDiscardReasonHangup:
				reasonMessage = fmt.Sprintf("call declined by %d", userId)
			}
			if ctx.p2pConfigs[userId] != nil {
				ctx.p2pConfigs[userId].WaitData <- fmt.Errorf(reasonMessage)
			}
			delete(ctx.inputCalls, userId)
			_ = ctx.binding.Stop(userId)
		case *tg.PhoneCallRequested:
			if ctx.p2pConfigs[userId] == nil {
				p2pConfigs, err := ctx.getP2PConfigs(call.GAHash)
				if err != nil {
					gologging.Error(err)
					return
				}
				ctx.p2pConfigs[userId] = p2pConfigs
				for _, callback := range ctx.incomingCallCallbacks {
					go callback(ctx, userId)
				}
			}
		}
	})

	ctx.app.OnRawUpdate(func(update *tg.UpdateGroupCallParticipants) {
		chatId, err := ctx.convertGroupCallId(update.Call.(*tg.InputGroupCall).ID)
		if err == nil {
			ctx.participantsMutex.Lock()
			if ctx.callParticipants[chatId] == nil {
				ctx.callParticipants[chatId] = &types.CallParticipantsCache{
					CallParticipants: make(map[int64]*tg.GroupCallParticipant),
				}
			}
			for _, participant := range update.Participants {
				participantId := parsePeer(participant.Peer)
				if participant.Left {
					delete(ctx.callParticipants[chatId].CallParticipants, participantId)
					if ctx.callSources != nil && ctx.callSources[chatId] != nil {
						delete(ctx.callSources[chatId].CameraSources, participantId)
						delete(ctx.callSources[chatId].ScreenSources, participantId)
					}
					continue
				}

				ctx.callParticipants[chatId].CallParticipants[participantId] = participant
				if ctx.callSources != nil && ctx.callSources[chatId] != nil {
					wasCamera := ctx.callSources[chatId].CameraSources[participantId] != ""
					wasScreen := ctx.callSources[chatId].ScreenSources[participantId] != ""

					if wasCamera != (participant.Video != nil) {
						if participant.Video != nil {
							ctx.callSources[chatId].CameraSources[participantId] = participant.Video.Endpoint
							_, _ = ctx.binding.AddIncomingVideo(
								chatId,
								participantId,
								participant.Video.Endpoint,
								parseVideoSources(participant.Video.SourceGroups),
							)
						} else {
							_ = ctx.binding.RemoveIncomingVideo(
								chatId,
								ctx.callSources[chatId].CameraSources[participantId],
							)
							delete(ctx.callSources[chatId].CameraSources, participantId)
						}
					}

					if wasScreen != (participant.Presentation != nil) {
						if participant.Presentation != nil {
							ctx.callSources[chatId].ScreenSources[participantId] = participant.Presentation.Endpoint
							_, _ = ctx.binding.AddIncomingVideo(
								chatId,
								participantId,
								participant.Presentation.Endpoint,
								parseVideoSources(participant.Presentation.SourceGroups),
							)
						} else {
							_ = ctx.binding.RemoveIncomingVideo(
								chatId,
								ctx.callSources[chatId].ScreenSources[participantId],
							)
							delete(ctx.callSources[chatId].ScreenSources, participantId)
						}
					}
				}
			}
			ctx.callParticipants[chatId].LastMTProtoUpdate = time.Now()
			ctx.participantsMutex.Unlock()

			for _, participant := range update.Participants {
				userPeer := participant.Peer.(*tg.PeerUser)
				if userPeer.UserID == ctx.self.UserID {
					connectionMode, err := ctx.binding.GetConnectionMode(chatId)
					if err == nil && connectionMode == ntgcalls.StreamConnection && participant.CanSelfUnmute {
						if ctx.pendingConnections[chatId] != nil {
							_ = ctx.connectCall(
								chatId,
								ctx.pendingConnections[chatId].MediaDescription,
								ctx.pendingConnections[chatId].Payload,
								false,
								nil,
							)
						}
					} else if !participant.CanSelfUnmute {
						if !slices.Contains(ctx.mutedByAdmin, chatId) {
							ctx.mutedByAdmin = append(ctx.mutedByAdmin, chatId)
						}
					} else if slices.Contains(ctx.mutedByAdmin, chatId) {
						state, err := ctx.binding.GetState(chatId)
						if err != nil {
							panic(err)
						}
						err = ctx.setCallStatus(update.Call, state)
						if err != nil {
							panic(err)
						}
						ctx.mutedByAdmin = stdRemove(ctx.mutedByAdmin, chatId)
					}
				}
			}
		}
	})

	ctx.app.OnRawUpdate(func(update *tg.UpdateGroupCall) {
		if groupCallRaw := update.Call; groupCallRaw != nil {
			chatID := parsePeer(update.Peer)
			switch groupCallRaw.(type) {
			case *tg.GroupCall:
				groupCall := groupCallRaw.(*tg.GroupCall)
				ctx.inputGroupCalls[chatID] = &tg.InputGroupCall{
					ID:         groupCall.ID,
					AccessHash: groupCall.AccessHash,
				}
				return
			case *tg.GroupCallDiscarded:
				delete(ctx.inputGroupCalls, chatID)
				_ = ctx.binding.Stop(chatID)
				return
			}
		}
	})

	ctx.app.OnRawUpdate(func(update *tg.UpdateGroupCallChainBlocks) {
		if inputCall, ok := update.Call.(*tg.InputGroupCall); ok {
			chatId, err := ctx.convertGroupCallId(inputCall.ID)
			if err == nil {
				_ = ctx.binding.ApplyBlocks(
					chatId,
					update.SubChainID,
					update.NextOffset,
					update.Blocks,
					false,
				)
			}
		}
	})

	ctx.binding.OnOutboundBlock(func(chatId int64, block []byte) {
		ctx.sendConferenceCallBroadcast(chatId, block)
	})

	ctx.binding.OnSubchainRequest(func(chatId int64, request ntgcalls.SubchainRequest) {
		blocks, nextOffset, err := ctx.getSubchainBlocks(chatId, request)
		if err == nil && len(blocks) > 0 {
			_ = ctx.binding.ApplyBlocks(chatId, request.Subchain, nextOffset, blocks, true)
		}
		_ = ctx.binding.FinishSubchainRequest(chatId, request.Subchain)
	})

	ctx.binding.OnRequestParticipants(func(chatId int64) {
		_, _ = ctx.GetParticipants(chatId)
	})

	ctx.binding.OnUpdateEmojis(func(chatId int64, emojis string) {
		for _, callback := range ctx.emojisCallbacks {
			go callback(chatId, emojis)
		}
	})

	ctx.binding.OnRequestBroadcastTimestamp(func(chatId int64) {
		if ctx.inputGroupCalls[chatId] != nil {
			channels, err := ctx.invoke(
				&tg.PhoneGetGroupCallStreamChannelsRequest{
					Call: ctx.inputGroupCalls[chatId],
				},
			)
			if err == nil {
				_ = ctx.binding.SendBroadcastTimestamp(
					chatId,
					channels.(*tg.PhoneGroupCallStreamChannels).Channels[0].LastTimestampMs,
				)
			}
		}
	})

	ctx.binding.OnRequestBroadcastPart(func(chatId int64, segmentPartRequest ntgcalls.SegmentPartRequest) {
		if ctx.inputGroupCalls[chatId] != nil {
			file, err := ctx.invoke(
				&tg.UploadGetFileRequest{
					Location: &tg.InputGroupCallStream{
						Call:         ctx.inputGroupCalls[chatId],
						TimeMs:       segmentPartRequest.Timestamp,
						Scale:        0,
						VideoChannel: segmentPartRequest.ChannelID,
						VideoQuality: max(int32(segmentPartRequest.Quality), 0),
					},
					Offset: 0,
					Limit:  segmentPartRequest.Limit,
				},
			)

			status := ntgcalls.SegmentStatusNotReady
			var data []byte
			data = nil

			if err != nil {
				if tgerr.IsFloodWait(err) {
					status = ntgcalls.SegmentStatusResyncNeeded
				}
			} else {
				data = file.(*tg.UploadFile).Bytes
				status = ntgcalls.SegmentStatusSuccess
			}

			_ = ctx.binding.SendBroadcastPart(
				chatId,
				segmentPartRequest.SegmentID,
				segmentPartRequest.PartID,
				status,
				segmentPartRequest.QualityUpdate,
				data,
			)
		}
	})

	ctx.binding.OnSignal(func(chatId int64, signal []byte) {
		_, _ = ctx.invoke(
			&tg.PhoneSendSignalingDataRequest{
				Peer: ctx.inputCalls[chatId],
				Data: signal,
			},
		)
	})

	ctx.binding.OnConnectionChange(func(chatId int64, state ntgcalls.NetworkInfo) {
		if ctx.waitConnect[chatId] != nil {
			switch state.State {
			case ntgcalls.Connected:
				ctx.waitConnect[chatId] <- nil
			case ntgcalls.Closed, ntgcalls.Failed:
				ctx.waitConnect[chatId] <- fmt.Errorf("connection failed")
			case ntgcalls.Timeout:
				ctx.waitConnect[chatId] <- fmt.Errorf("connection timeout")
			default:
			}
		}
	})

	ctx.binding.OnUpgrade(func(chatId int64, state ntgcalls.MediaState) {
		err := ctx.setCallStatus(ctx.inputGroupCalls[chatId], state)
		if err != nil {
			gologging.ErrorF("%v", err)
		}
	})

	ctx.binding.OnStreamEnd(func(chatId int64, streamType ntgcalls.StreamType, streamDevice ntgcalls.StreamDevice) {
		for _, callback := range ctx.streamEndCallbacks {
			go callback(chatId, streamType, streamDevice)
		}
	})

	ctx.binding.OnFrame(func(chatId int64, mode ntgcalls.StreamMode, device ntgcalls.StreamDevice, frames []ntgcalls.Frame) {
		for _, callback := range ctx.frameCallbacks {
			go callback(chatId, mode, device, frames)
		}
	})
}
