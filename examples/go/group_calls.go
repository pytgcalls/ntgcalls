package main

/*
func joinGroupCall(client *ntgcalls.Client, mtproto *tg.Client, username, url string) {
	me, _ := mtproto.GetMe()
	rawChannel, err := mtproto.ResolveUsername(username)
	if err != nil {
		panic(err)
	}
	channel := rawChannel.(*tg.Channel)
	mediaDescription := getMediaDescription(url)
	jsonParams, err := client.CreateCall(channel.ID)
	_ = client.SetStreamSources(channel.ID, ntgcalls.CaptureStream, mediaDescription)
	if err != nil {
		panic(err)
	}
	fullChatRaw, err := mtproto.ChannelsGetFullChannel(
		&tg.InputChannelObj{
			ChannelID:  channel.ID,
			AccessHash: channel.AccessHash,
		},
	)
	if err != nil {
		panic(err)
	}
	fullChat := fullChatRaw.FullChat.(*tg.ChannelFull)
	if fullChat.Call == nil {
		panic("No group call found")
	}
	client.OnRequestBroadcastTimestamp(func(chatId int64) {
		channels, err := mtproto.PhoneGetGroupCallStreamChannels(fullChat.Call)
		if err == nil {
			_ = client.SendBroadcastTimestamp(chatId, channels.Channels[0].LastTimestampMs)
		}
	})
	client.OnRequestBroadcastPart(func(chatId int64, segmentPartRequest ntgcalls.SegmentPartRequest) {
		file, err := mtproto.UploadGetFile(
			&tg.UploadGetFileParams{
				Location: &tg.InputGroupCallStream{
					Call:         fullChat.Call,
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
			secondsWait := tg.GetFloodWait(err)
			if secondsWait == 0 {
				status = ntgcalls.SegmentStatusResyncNeeded
			}
		} else {
			data = file.(*tg.UploadFileObj).Bytes
			status = ntgcalls.SegmentStatusSuccess
		}
		_ = client.SendBroadcastPart(
			chatId,
			segmentPartRequest.SegmentID,
			segmentPartRequest.PartID,
			status,
			segmentPartRequest.QualityUpdate,
			data,
		)
	})
	mtproto.AddRawHandler(&tg.UpdateGroupCallParticipants{}, func(m tg.Update, c *tg.Client) error {
		participants := m.(*tg.UpdateGroupCallParticipants).Participants
		for _, participant := range participants {
			userPeer := participant.Peer.(*tg.PeerUser)
			if userPeer.UserID == me.ID {
				connectionMode, err := client.GetConnectionMode(channel.ID)
				if err == nil && connectionMode == ntgcalls.StreamConnection && participant.CanSelfUnmute {
					// We need to rejoin the group call after the user is unmuted in channels with more
					// than 20 participants in the group call.
					_, err = joinMtprotoGroupCall(client, mtproto, channel.ID, jsonParams, fullChat.Call, me)
					if err != nil {
						panic(err)
					}
				} else if !participant.CanSelfUnmute {
					mutedByAdmin[channel.ID] = true
				} else if mutedByAdmin[channel.ID] {
					// If the admin unmutes the user, we need to edit ourselves to allow the user to
					// stream the video again.
					state, err := client.GetState(channel.ID)
					if err != nil {
						panic(err)
					}
					err = setCallStatus(mtproto, fullChat.Call, me, state)
					if err != nil {
						panic(err)
					}
					delete(mutedByAdmin, channel.ID)
				}

				if pendingPresentation[channel.ID] && participant.CanSelfUnmute {
					err = joinPresentation(client, mtproto, channel.ID, fullChat.Call)
					if err != nil {
						panic(err)
					}
					delete(pendingPresentation, channel.ID)
				}
			}
		}
		return nil
	})
	client.OnUpgrade(func(chatId int64, state ntgcalls.MediaState) {
		err := setCallStatus(mtproto, fullChat.Call, me, state)
		if err != nil {
			panic(err)
		}
	})
	joinMuted, err := joinMtprotoGroupCall(client, mtproto, channel.ID, jsonParams, fullChat.Call, me)
	if err != nil {
		panic(err)
	}
	if mediaDescription.Screen != nil {
		connectionMode, err := client.GetConnectionMode(channel.ID)
		if err != nil {
			panic(err)
		}
		if connectionMode == ntgcalls.StreamConnection || joinMuted {
			pendingPresentation[channel.ID] = true
		} else {
			err = joinPresentation(client, mtproto, channel.ID, fullChat.Call)
			if err != nil {
				panic(err)
			}
		}
	}
	err = client.SetStreamSources(channel.ID, ntgcalls.PlaybackStream, ntgcalls.MediaDescription{
		Microphone: &ntgcalls.AudioDescription{
			MediaSource:  ntgcalls.MediaSourceDevice,
			SampleRate:   96000,
			ChannelCount: 2,
			Input:        ntgcalls.GetMediaDevices().Speaker[2].Metadata,
		},
	})
	if err != nil {
		panic(err)
	}
}

func joinPresentation(client *ntgcalls.Client, mtproto *tg.Client, channelId int64, call *tg.InputGroupCall) error {
	presentationParams, err := client.InitPresentation(channelId)
	if err != nil {
		return err
	}
	callResRaw, err := mtproto.PhoneJoinGroupCallPresentation(
		call,
		&tg.DataJson{
			Data: presentationParams,
		},
	)
	if err != nil {
		return err
	}
	callRes := callResRaw.(*tg.UpdatesObj)
	for _, update := range callRes.Updates {
		switch update.(type) {
		case *tg.UpdateGroupCallConnection:
			phoneCall := update.(*tg.UpdateGroupCallConnection)
			_ = client.Connect(channelId, phoneCall.Params.Data, true)
		}
	}
	return nil
}

func joinMtprotoGroupCall(client *ntgcalls.Client, mtproto *tg.Client, channelId int64, jsonParams string, call *tg.InputGroupCall, me *tg.UserObj) (bool, error) {
	state, err := client.GetState(channelId)
	if err != nil {
		return false, err
	}
	callResRaw, err := mtproto.PhoneJoinGroupCall(
		&tg.PhoneJoinGroupCallParams{
			Muted:        false,
			VideoStopped: state.VideoStopped,
			Call:         call,
			Params: &tg.DataJson{
				Data: jsonParams,
			},
			JoinAs: &tg.InputPeerUser{
				UserID:     me.ID,
				AccessHash: me.AccessHash,
			},
		},
	)
	if err != nil {
		return false, err
	}
	callRes := callResRaw.(*tg.UpdatesObj)
	joinMuted := false
	for _, update := range callRes.Updates {
		switch update.(type) {
		case *tg.UpdateGroupCallConnection:
			phoneCall := update.(*tg.UpdateGroupCallConnection)
			err = client.Connect(channelId, phoneCall.Params.Data, false)
			if err != nil {
				return false, err
			}
		case *tg.UpdateGroupCallParticipants:
			participants := update.(*tg.UpdateGroupCallParticipants).Participants
			for _, participant := range participants {
				userPeer := participant.Peer.(*tg.PeerUser)
				if userPeer.UserID == me.ID {
					joinMuted = !participant.CanSelfUnmute
				}
			}
		}
	}
	return joinMuted, nil
}

func setCallStatus(mtproto *tg.Client, call *tg.InputGroupCall, me *tg.UserObj, state ntgcalls.MediaState) error {
	_, err := mtproto.PhoneEditGroupCallParticipant(
		&tg.PhoneEditGroupCallParticipantParams{
			Call: call,
			Participant: &tg.InputPeerUser{
				UserID:     me.ID,
				AccessHash: me.AccessHash,
			},
			Muted:              state.Muted,
			VideoPaused:        state.VideoPaused,
			VideoStopped:       state.VideoStopped,
			PresentationPaused: state.PresentationPaused,
		},
	)
	return err
}
*/
