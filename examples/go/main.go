package main

/*
// You can also change the file names to have multiple libraries on the same folder,
// but you need to change the name of the library in the LDFLAGS and the -L folder from . to the
// folder where the library is located.
#cgo linux LDFLAGS: -L . -lntgcalls -lm -lz
#cgo darwin LDFLAGS: -L . -lntgcalls -lc++ -liconv -framework AVFoundation -framework AudioToolbox -framework CoreAudio -framework QuartzCore -framework CoreMedia -framework VideoToolbox -framework AppKit -framework Metal -framework MetalKit -framework OpenGL -framework IOSurface -framework ScreenCaptureKit

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

func main() {
	client := ntgcalls.NTgCalls()
	defer client.Free()
	mtproto, _ := tg.NewClient(tg.ClientConfig{
		AppID:   10029733,
		AppHash: "d0d81009d46e774f78c0e0e622f5fa21",
		Session: "session",
	})
	_ = mtproto.Start()
	// Choose between outgoingCall or joinGroupCall
	//outgoingCall(client, mtproto, "@Laky64")
	//joinGroupCall(client, mtproto, "@pytgcallschat")
	fmt.Println(client.Calls())
	client.OnStreamEnd(func(chatId int64, streamType ntgcalls.StreamType, streamDevice ntgcalls.StreamDevice) {
		fmt.Println(chatId)
	})
	client.OnConnectionChange(func(chatId int64, state ntgcalls.NetworkInfo) {
		switch state.State {
		case ntgcalls.Connecting:
			fmt.Println("Connecting with chatId:", chatId)
		case ntgcalls.Connected:
			fmt.Println("Connected with chatId:", chatId)
		case ntgcalls.Failed:
			fmt.Println("Failed with chatId:", chatId)
		case ntgcalls.Timeout:
			fmt.Println("Timeout with chatId:", chatId)
		case ntgcalls.Closed:
			fmt.Println("Closed with chatId:", chatId)
		}
	})
	mtproto.Idle()
}

func joinGroupCall(client *ntgcalls.Client, mtproto *tg.Client, username string) {
	me, _ := mtproto.GetMe()
	rawChannel, _ := mtproto.ResolveUsername(username)
	channel := rawChannel.(*tg.Channel)
	jsonParams, _ := client.CreateCall(channel.ID, ntgcalls.MediaDescription{
		Microphone: &ntgcalls.AudioDescription{
			MediaSource:  ntgcalls.MediaSourceShell,
			SampleRate:   96000,
			ChannelCount: 2,
			Input:        "ffmpeg -reconnect 1 -reconnect_at_eof 1 -reconnect_streamed 1 -reconnect_delay_max 2 -i https://docs.evostream.com/sample_content/assets/sintel1m720p.mp4 -f s16le -ac 2 -ar 96k -v quiet pipe:1",
		},
	})
	fullChatRaw, _ := mtproto.ChannelsGetFullChannel(
		&tg.InputChannelObj{
			ChannelID:  channel.ID,
			AccessHash: channel.AccessHash,
		},
	)
	fullChat := fullChatRaw.FullChat.(*tg.ChannelFull)
	callResRaw, _ := mtproto.PhoneJoinGroupCall(
		&tg.PhoneJoinGroupCallParams{
			Muted:        false,
			VideoStopped: true,
			Call:         fullChat.Call,
			Params: &tg.DataJson{
				Data: jsonParams,
			},
			JoinAs: &tg.InputPeerUser{
				UserID:     me.ID,
				AccessHash: me.AccessHash,
			},
		},
	)
	callRes := callResRaw.(*tg.UpdatesObj)
	for _, update := range callRes.Updates {
		switch update.(type) {
		case *tg.UpdateGroupCallConnection:
			phoneCall := update.(*tg.UpdateGroupCallConnection)
			_ = client.Connect(channel.ID, phoneCall.Params.Data, false)
		}
	}
}

func outgoingCall(client *ntgcalls.Client, mtproto *tg.Client, username string) {
	rawUser, _ := mtproto.ResolveUsername(username)
	user := rawUser.(*tg.UserObj)
	dhConfigRaw, _ := mtproto.MessagesGetDhConfig(0, 256)
	dhConfig := dhConfigRaw.(*tg.MessagesDhConfigObj)
	_ = client.CreateP2PCall(user.ID, ntgcalls.MediaDescription{
		Microphone: &ntgcalls.AudioDescription{
			MediaSource:  ntgcalls.MediaSourceShell,
			SampleRate:   96000,
			ChannelCount: 2,
			Input:        "ffmpeg -reconnect 1 -reconnect_at_eof 1 -reconnect_streamed 1 -reconnect_delay_max 2 -i https://docs.evostream.com/sample_content/assets/sintel1m720p.mp4 -f s16le -ac 2 -ar 96k -v quiet pipe:1",
		},
	})
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
