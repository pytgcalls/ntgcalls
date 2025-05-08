package main

/*
// You can also change the file names to have multiple libraries on the same folder,
// but you need to change the name of the library in the LDFLAGS and the -L folder from . to the
// folder where the library is located.
#cgo linux LDFLAGS: -L . -lntgcalls -lm -lz
#cgo darwin LDFLAGS: -L . -lntgcalls -lc++ -lz -lbz2 -liconv -framework AVFoundation -framework AudioToolbox -framework CoreAudio -framework QuartzCore -framework CoreMedia -framework VideoToolbox -framework AppKit -framework Metal -framework MetalKit -framework OpenGL -framework IOSurface -framework ScreenCaptureKit

// Currently is supported only dynamically linked library on Windows due to
// https://github.com/golang/go/issues/63903
#cgo windows LDFLAGS: -L. -lntgcalls
#include "ntgcalls/ntgcalls.h"
#include "glibc_compatibility.h"
*/
import "C"
import (
	"fmt"
	tg "github.com/amarnathcjd/gogram/telegram"
	"gotgcalls/ntgcalls"
)

var inputCall *tg.InputPhoneCall
var mutedByAdmin map[int64]bool
var pendingPresentation map[int64]bool
var urlVideoTest = "https://docs.evostream.com/sample_content/assets/sintel1m720p.mp4"

func main() {
	client := ntgcalls.NTgCalls()
	defer client.Free()
	mutedByAdmin = make(map[int64]bool)
	pendingPresentation = make(map[int64]bool)
	mtproto, _ := tg.NewClient(tg.ClientConfig{
		AppID:   10029733,
		AppHash: "d0d81009d46e774f78c0e0e622f5fa21",
		Session: "session",
	})
	_ = mtproto.Start()
	// Choose between outgoingCall or joinGroupCall
	//outgoingCall(client, mtproto, "@Laky64", urlVideoTest)
	//joinGroupCall(client, mtproto, "@pytgcallschat", urlVideoTest)
	fmt.Println(client.Calls())
	client.OnStreamEnd(func(chatId int64, streamType ntgcalls.StreamType, streamDevice ntgcalls.StreamDevice) {
		fmt.Println("Stream ended with chatId:", chatId, "streamType:", streamType, "streamDevice:", streamDevice)
	})
	client.OnConnectionChange(func(chatId int64, state ntgcalls.NetworkInfo) {
		var connectionName string
		if state.Kind == ntgcalls.NormalConnection {
			connectionName = "Normal"
		} else if state.Kind == ntgcalls.PresentationConnection {
			connectionName = "Presentation"
		}
		switch state.State {
		case ntgcalls.Connecting:
			fmt.Println("Connecting with chatId:", chatId, "connection:", connectionName)
		case ntgcalls.Connected:
			fmt.Println("Connected with chatId:", chatId, "connection:", connectionName)
		case ntgcalls.Failed:
			fmt.Println("Failed with chatId:", chatId, "connection:", connectionName)
		case ntgcalls.Timeout:
			fmt.Println("Timeout with chatId:", chatId, "connection:", connectionName)
		case ntgcalls.Closed:
			fmt.Println("Closed with chatId:", chatId, "connection:", connectionName)
		}
	})
	mtproto.Idle()
}

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

func outgoingCall(client *ntgcalls.Client, mtproto *tg.Client, username string, url string) {
	rawUser, _ := mtproto.ResolveUsername(username)
	user := rawUser.(*tg.UserObj)
	dhConfigRaw, _ := mtproto.MessagesGetDhConfig(0, 256)
	dhConfig := dhConfigRaw.(*tg.MessagesDhConfigObj)
	_ = client.CreateP2PCall(user.ID)
	_ = client.SetStreamSources(user.ID, ntgcalls.CaptureStream, getMediaDescription(url))
	gAHash, _ := client.InitExchange(user.ID, ntgcalls.DhConfig{
		G:      dhConfig.G,
		P:      dhConfig.P,
		Random: dhConfig.Random,
	}, nil)
	protocolRaw := ntgcalls.GetProtocol()
	protocol := &tg.PhoneCallProtocol{
		UdpP2P:          protocolRaw.UdpP2P,
		UdpReflector:    protocolRaw.UdpReflector,
		MinLayer:        protocolRaw.MinLayer,
		MaxLayer:        protocolRaw.MaxLayer,
		LibraryVersions: protocolRaw.Versions,
	}
	_, _ = mtproto.PhoneRequestCall(
		&tg.PhoneRequestCallParams{
			Protocol: protocol,
			UserID:   &tg.InputUserObj{UserID: user.ID, AccessHash: user.AccessHash},
			GAHash:   gAHash,
			RandomID: int32(tg.GenRandInt()),
		},
	)

	mtproto.AddRawHandler(&tg.UpdatePhoneCall{}, func(m tg.Update, c *tg.Client) error {
		phoneCall := m.(*tg.UpdatePhoneCall).PhoneCall
		switch phoneCall.(type) {
		case *tg.PhoneCallAccepted:
			call := phoneCall.(*tg.PhoneCallAccepted)
			res, _ := client.ExchangeKeys(user.ID, call.GB, 0)
			inputCall = &tg.InputPhoneCall{
				ID:         call.ID,
				AccessHash: call.AccessHash,
			}
			client.OnSignal(func(chatId int64, signal []byte) {
				_, _ = mtproto.PhoneSendSignalingData(inputCall, signal)
			})
			callConfirmRes, _ := mtproto.PhoneConfirmCall(
				inputCall,
				res.GAOrB,
				res.KeyFingerprint,
				protocol,
			)
			callRes := callConfirmRes.PhoneCall.(*tg.PhoneCallObj)
			rtcServers := make([]ntgcalls.RTCServer, len(callRes.Connections))
			for i, connection := range callRes.Connections {
				switch connection.(type) {
				case *tg.PhoneConnectionWebrtc:
					rtcServer := connection.(*tg.PhoneConnectionWebrtc)
					rtcServers[i] = ntgcalls.RTCServer{
						ID:       rtcServer.ID,
						Ipv4:     rtcServer.Ip,
						Ipv6:     rtcServer.Ipv6,
						Username: rtcServer.Username,
						Password: rtcServer.Password,
						Port:     rtcServer.Port,
						Turn:     rtcServer.Turn,
						Stun:     rtcServer.Stun,
					}
				case *tg.PhoneConnectionObj:
					phoneServer := connection.(*tg.PhoneConnectionObj)
					rtcServers[i] = ntgcalls.RTCServer{
						ID:      phoneServer.ID,
						Ipv4:    phoneServer.Ip,
						Ipv6:    phoneServer.Ipv6,
						Port:    phoneServer.Port,
						Turn:    true,
						Tcp:     phoneServer.Tcp,
						PeerTag: phoneServer.PeerTag,
					}
				}
			}
			_ = client.ConnectP2P(user.ID, rtcServers, callRes.Protocol.LibraryVersions, callRes.P2PAllowed)
		}
		return nil
	})

	mtproto.AddRawHandler(&tg.UpdatePhoneCallSignalingData{}, func(m tg.Update, c *tg.Client) error {
		signalingData := m.(*tg.UpdatePhoneCallSignalingData).Data
		_ = client.SendSignalingData(user.ID, signalingData)
		return nil
	})
}

func getMediaDescription(url string) ntgcalls.MediaDescription {
	audioDescription := &ntgcalls.AudioDescription{
		MediaSource:  ntgcalls.MediaSourceShell,
		SampleRate:   96000,
		ChannelCount: 2,
	}
	videoDescription := &ntgcalls.VideoDescription{
		MediaSource: ntgcalls.MediaSourceShell,
		Width:       1280,
		Height:      720,
		Fps:         30,
	}

	baseFFmpeg := "ffmpeg -reconnect 1 -reconnect_at_eof 1 -reconnect_streamed 1 -reconnect_delay_max 2 -i"
	audioDescription.Input = fmt.Sprintf(
		"%s %s -f s16le -ac %d -ar %d -v quiet pipe:1",
		baseFFmpeg,
		url,
		audioDescription.ChannelCount,
		audioDescription.SampleRate,
	)
	videoDescription.Input = fmt.Sprintf(
		"%s %s -f rawvideo -r %d -pix_fmt yuv420p -vf scale=%d:%d -v quiet pipe:1",
		baseFFmpeg,
		url,
		videoDescription.Fps,
		videoDescription.Width,
		videoDescription.Height,
	)
	return ntgcalls.MediaDescription{
		Microphone: audioDescription,
		Screen:     videoDescription,
	}
}
