package ubot

import (
	"fmt"
	"gotgcalls/ntgcalls"
	"gotgcalls/ubot/types"
	"time"

	"github.com/mtgo-labs/mtgo/tg"
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
		conferenceParams, err := ctx.binding.InitConference(chatId, ctx.self.UserID, lastBlock)
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

		var rawUpdates []tg.UpdateClass
		if lastBlock == nil {
			callResRaw, err := ctx.invoke(
				&tg.PhoneCreateConferenceCallRequest{
					Muted:        false,
					VideoStopped: state.VideoStopped,
					Join:         true,
					RandomID:     int32(ctx.app.RandomID()),
					Block:        conferenceParams.Block,
					Params: &tg.DataJSON{
						Data: conferenceParams.Payload,
					},
					PublicKey: (*[32]byte)(conferenceParams.PublicKey),
				},
			)
			if err != nil {
				return err
			}
			rawUpdates = callResRaw.(*tg.Updates).Updates
		} else {
			inputCall, err := ctx.getInputGroupCall(chatId)
			if err != nil {
				return err
			}
			callResRaw, err := ctx.invoke(
				&tg.PhoneJoinGroupCallRequest{
					Muted:        false,
					VideoStopped: state.VideoStopped,
					Call:         inputCall,
					JoinAs:       ctx.self,
					Block:        conferenceParams.Block,
					Params: &tg.DataJSON{
						Data: conferenceParams.Payload,
					},
					PublicKey: (*[32]byte)(conferenceParams.PublicKey),
				},
			)
			if err != nil {
				return err
			}
			rawUpdates = callResRaw.(*tg.Updates).Updates
		}

		resultParams := "{\"transport\": null}"
		for _, update := range rawUpdates {
			switch u := update.(type) {
			case *tg.UpdateGroupCall:
				if groupCall, ok := u.Call.(*tg.GroupCall); ok {
					ctx.inputGroupCalls[chatId] = &tg.InputGroupCall{
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
			inputUser, err := ctx.getSendableUser(chatId)
			if err != nil {
				return err
			}
			_, err = ctx.invoke(
				&tg.PhoneInviteConferenceCallParticipantRequest{
					Video:  !state.VideoStopped,
					Call:   inputCall,
					UserID: inputUser,
				},
			)
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
			UDPP2p:          protocolRaw.UdpP2P,
			UDPReflector:    protocolRaw.UdpReflector,
			MinLayer:        protocolRaw.MinLayer,
			MaxLayer:        protocolRaw.MaxLayer,
			LibraryVersions: protocolRaw.Versions,
		}

		userId, err := ctx.getSendableUser(chatId)
		if err != nil {
			return err
		}
		if ctx.p2pConfigs[chatId].IsOutgoing {
			_, err = ctx.invoke(
				&tg.PhoneRequestCallRequest{
					Protocol: protocol,
					UserID:   userId,
					GAHash:   ctx.p2pConfigs[chatId].GAorB,
					RandomID: int32(ctx.app.RandomID()),
					Video:    mediaDescription.Camera != nil || mediaDescription.Screen != nil,
				},
			)
			if err != nil {
				return err
			}
		} else {
			_, err = ctx.invoke(
				&tg.PhoneAcceptCallRequest{
					Peer:     ctx.inputCalls[chatId],
					GB:       ctx.p2pConfigs[chatId].GAorB,
					Protocol: protocol,
				},
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
			confirmRes, err := ctx.invoke(
				&tg.PhoneConfirmCallRequest{
					Peer:           ctx.inputCalls[chatId],
					GA:             res.GAOrB,
					KeyFingerprint: res.KeyFingerprint,
					Protocol:       protocol,
				},
			)
			if err != nil {
				return err
			}
			ctx.p2pConfigs[chatId].PhoneCall = confirmRes.(*tg.PhonePhoneCall).PhoneCall.(*tg.PhoneCall)
		}

		err = ctx.binding.ConnectP2P(
			chatId,
			parseRTCServers(ctx.p2pConfigs[chatId].PhoneCall.Connections),
			ctx.p2pConfigs[chatId].PhoneCall.Protocol.LibraryVersions,
			ctx.p2pConfigs[chatId].PhoneCall.P2pAllowed,
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
		callRes, err := ctx.invoke(
			&tg.PhoneJoinGroupCallRequest{
				Muted:        false,
				VideoStopped: mediaDescription.Camera == nil,
				Call:         inputGroupCall,
				Params: &tg.DataJSON{
					Data: jsonParams,
				},
				JoinAs: ctx.self,
			},
		)
		if err != nil {
			return err
		}
		for _, update := range callRes.(*tg.Updates).Updates {
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
