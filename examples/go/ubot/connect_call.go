package ubot

import (
	"fmt"
	"gotgcalls/ntgcalls"
	"gotgcalls/ubot/types"
	"time"

	tg "github.com/amarnathcjd/gogram/telegram"
)

func (ctx *Context) connectCall(chatId int64, mediaDescription ntgcalls.MediaDescription, jsonParams string, conference bool, lastBlock []byte) error {
	defer func() {
		if ctx.waitConnect[chatId] != nil {
			delete(ctx.waitConnect, chatId)
		}
		delete(ctx.pendingConnections, chatId)
	}()
	ctx.waitConnect[chatId] = make(chan error)
	if conference {
		if lastBlock == nil {
			if err := ctx.binding.CreateP2PCall(chatId); err != nil {
				return err
			}
		}
		conferenceParams, err := ctx.binding.InitConference(chatId, ctx.self.ID, lastBlock)
		if err != nil {
			return err
		}
		if lastBlock == nil {
			if err = ctx.binding.SetStreamSources(chatId, ntgcalls.CaptureStream, mediaDescription); err != nil {
				return err
			}
		}
		state, err := ctx.binding.GetState(chatId)
		if err != nil {
			return err
		}

		var rawUpdates []tg.Update
		if lastBlock == nil {
			createParams := &tg.PhoneCreateConferenceCallParams{
				Muted:        false,
				VideoStopped: state.VideoStopped,
				Join:         true,
				RandomID:     int32(tg.GenRandInt()),
				Block:        conferenceParams.Block,
				Params: &tg.DataJson{
					Data: conferenceParams.Payload,
				},
				PublicKey: tg.NewInt256(conferenceParams.PublicKey),
			}
			callResRaw, err := ctx.app.PhoneCreateConferenceCall(createParams)
			if err != nil {
				return err
			}
			rawUpdates = callResRaw.(*tg.UpdatesObj).Updates
		} else {
			inputCall, err := ctx.getInputGroupCall(chatId)
			if err != nil {
				return err
			}
			joinParams := &tg.PhoneJoinGroupCallParams{
				Muted:        false,
				VideoStopped: state.VideoStopped,
				Call:         inputCall,
				JoinAs: &tg.InputPeerUser{
					UserID:     ctx.self.ID,
					AccessHash: ctx.self.AccessHash,
				},
				Block: conferenceParams.Block,
				Params: &tg.DataJson{
					Data: conferenceParams.Payload,
				},
				PublicKey: tg.NewInt256(conferenceParams.PublicKey),
			}
			callResRaw, err := ctx.app.PhoneJoinGroupCall(joinParams)
			if err != nil {
				return err
			}
			rawUpdates = callResRaw.(*tg.UpdatesObj).Updates
		}

		resultParams := "{\"transport\": null}"
		for _, update := range rawUpdates {
			switch u := update.(type) {
			case *tg.UpdateGroupCall:
				if groupCall, ok := u.Call.(*tg.GroupCallObj); ok {
					ctx.inputGroupCalls[chatId] = &tg.InputGroupCallObj{
						ID:         groupCall.ID,
						AccessHash: groupCall.AccessHash,
					}
				}
			case *tg.UpdateGroupCallConnection:
				resultParams = u.Params.Data
			}
		}

		if lastBlock == nil {
			inputCall, err := ctx.getInputGroupCall(chatId)
			if err != nil {
				return err
			}
			inputUser, err := ctx.app.GetSendableUser(chatId)
			if err != nil {
				return err
			}
			_, err = ctx.app.PhoneInviteConferenceCallParticipant(!state.VideoStopped, inputCall, inputUser)
			if err != nil {
				return err
			}
		}

		if err = ctx.binding.Connect(chatId, resultParams, false); err != nil {
			return err
		}
	} else if chatId >= 0 {
		defer func() {
			if ctx.p2pConfigs[chatId] != nil {
				delete(ctx.p2pConfigs, chatId)
			}
		}()
		if ctx.p2pConfigs[chatId] == nil {
			p2pConfigs, err := ctx.getP2PConfigs(nil)
			if err != nil {
				return err
			}
			ctx.p2pConfigs[chatId] = p2pConfigs
		}

		err := ctx.binding.CreateP2PCall(chatId)
		if err != nil {
			return err
		}

		err = ctx.binding.SetStreamSources(chatId, ntgcalls.CaptureStream, mediaDescription)
		if err != nil {
			return err
		}

		ctx.p2pConfigs[chatId].GAorB, err = ctx.binding.InitExchange(chatId, ntgcalls.DhConfig{
			G:      ctx.p2pConfigs[chatId].DhConfig.G,
			P:      ctx.p2pConfigs[chatId].DhConfig.P,
			Random: ctx.p2pConfigs[chatId].DhConfig.Random,
		}, ctx.p2pConfigs[chatId].GAorB)
		if err != nil {
			return err
		}

		protocolRaw := ntgcalls.GetProtocol()
		protocol := &tg.PhoneCallProtocol{
			UdpP2P:          protocolRaw.UdpP2P,
			UdpReflector:    protocolRaw.UdpReflector,
			MinLayer:        protocolRaw.MinLayer,
			MaxLayer:        protocolRaw.MaxLayer,
			LibraryVersions: protocolRaw.Versions,
		}

		userId, err := ctx.app.GetSendableUser(chatId)
		if err != nil {
			return err
		}
		if ctx.p2pConfigs[chatId].IsOutgoing {
			_, err = ctx.app.PhoneRequestCall(
				&tg.PhoneRequestCallParams{
					Protocol: protocol,
					UserID:   userId,
					GAHash:   ctx.p2pConfigs[chatId].GAorB,
					RandomID: int32(tg.GenRandInt()),
					Video:    mediaDescription.Camera != nil || mediaDescription.Screen != nil,
				},
			)
			if err != nil {
				return err
			}
		} else {
			_, err = ctx.app.PhoneAcceptCall(
				ctx.inputCalls[chatId],
				ctx.p2pConfigs[chatId].GAorB,
				protocol,
			)
			if err != nil {
				return err
			}
		}
		select {
		case err = <-ctx.p2pConfigs[chatId].WaitData:
			if err != nil {
				return err
			}
		case <-time.After(10 * time.Second):
			return fmt.Errorf("timed out waiting for an answer")
		}
		res, err := ctx.binding.ExchangeKeys(
			chatId,
			ctx.p2pConfigs[chatId].GAorB,
			ctx.p2pConfigs[chatId].KeyFingerprint,
		)
		if err != nil {
			return err
		}

		if ctx.p2pConfigs[chatId].IsOutgoing {
			confirmRes, err := ctx.app.PhoneConfirmCall(
				ctx.inputCalls[chatId],
				res.GAOrB,
				res.KeyFingerprint,
				protocol,
			)
			if err != nil {
				return err
			}
			ctx.p2pConfigs[chatId].PhoneCall = confirmRes.PhoneCall.(*tg.PhoneCallObj)
		}

		err = ctx.binding.ConnectP2P(
			chatId,
			parseRTCServers(ctx.p2pConfigs[chatId].PhoneCall.Connections),
			ctx.p2pConfigs[chatId].PhoneCall.Protocol.LibraryVersions,
			ctx.p2pConfigs[chatId].PhoneCall.P2PAllowed,
			"",
		)
		if err != nil {
			return err
		}
	} else {
		var err error
		jsonParams, err = ctx.binding.CreateCall(chatId)
		if err != nil {
			_ = ctx.binding.Stop(chatId)
			return err
		}

		err = ctx.binding.SetStreamSources(chatId, ntgcalls.CaptureStream, mediaDescription)
		if err != nil {
			_ = ctx.binding.Stop(chatId)
			return err
		}

		inputGroupCall, err := ctx.getInputGroupCall(chatId)
		if err != nil {
			_ = ctx.binding.Stop(chatId)
			return err
		}

		resultParams := "{\"transport\": null}"
		callResRaw, err := ctx.app.PhoneJoinGroupCall(
			&tg.PhoneJoinGroupCallParams{
				Muted:        false,
				VideoStopped: mediaDescription.Camera == nil,
				Call:         inputGroupCall,
				Params: &tg.DataJson{
					Data: jsonParams,
				},
				JoinAs: &tg.InputPeerUser{
					UserID:     ctx.self.ID,
					AccessHash: ctx.self.AccessHash,
				},
			},
		)
		if err != nil {
			return err
		}
		callRes := callResRaw.(*tg.UpdatesObj)
		for _, update := range callRes.Updates {
			switch update.(type) {
			case *tg.UpdateGroupCallConnection:
				resultParams = update.(*tg.UpdateGroupCallConnection).Params.Data
			}
		}

		err = ctx.binding.Connect(
			chatId,
			resultParams,
			false,
		)
		if err != nil {
			return err
		}

		connectionMode, err := ctx.binding.GetConnectionMode(chatId)
		if err != nil {
			return err
		}

		if connectionMode == ntgcalls.StreamConnection && len(jsonParams) > 0 {
			ctx.pendingConnections[chatId] = &types.PendingConnection{
				MediaDescription: mediaDescription,
				Payload:          jsonParams,
			}
		}
	}

	if err := <-ctx.waitConnect[chatId]; err != nil {
		return err
	}

	if conference || chatId < 0 {
		state, err := ctx.binding.GetState(chatId)
		if err != nil {
			return err
		}
		if err = ctx.joinPresentation(chatId, !state.PresentationStopped); err != nil {
			return err
		}
		if ctx.callSources[chatId] == nil {
			ctx.callSources[chatId] = &types.CallSources{
				CameraSources: make(map[int64]string),
				ScreenSources: make(map[int64]string),
			}
		}
		return ctx.updateSources(chatId)
	}
	return nil
}
