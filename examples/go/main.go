package main

//#cgo LDFLAGS: -L . -lntgcalls -Wl,-rpath=./
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
	p2pOutCall(client, mtproto, "@PyTgCallsVideoBeta")
	mtproto.Idle()
	client.OnStreamEnd(func(chatId int64, streamType ntgcalls.StreamType) {
		fmt.Println(chatId)
	})
}

func p2pOutCall(client *ntgcalls.Client, mtproto *tg.Client, username string) {
	rawUser, _ := mtproto.ResolveUsername(username)
	user := rawUser.(*tg.UserObj)
	dhConfigRaw, _ := mtproto.MessagesGetDhConfig(0, 256)
	dhConfig := dhConfigRaw.(*tg.MessagesDhConfigObj)
	gAHash, _ := client.CreateP2PCall(user.ID, dhConfig.G, dhConfig.P, dhConfig.Random, nil, ntgcalls.MediaDescription{
		Audio: &ntgcalls.AudioDescription{
			InputMode:     ntgcalls.InputModeShell,
			SampleRate:    96000,
			BitsPerSample: 16,
			ChannelCount:  2,
			Input:         "ffmpeg -i https://docs.evostream.com/sample_content/assets/sintel1m720p.mp4 -f s16le -ac 2 -ar 96k -v quiet pipe:1",
		},
	})
	protocolRaw := client.GetProtocol()
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
			res, _ := client.ExchangeKeys(user.ID, dhConfig.P, call.GB, 0)
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
			go func(rtcServers []ntgcalls.RTCServer, versions []string) {
				_ = client.ConnectP2P(user.ID, rtcServers, versions, callRes.P2PAllowed)
			}(rtcServers, callRes.Protocol.LibraryVersions)
		}
		return nil
	})

	mtproto.AddRawHandler(&tg.UpdatePhoneCallSignalingData{}, func(m tg.Update, c *tg.Client) error {
		signalingData := m.(*tg.UpdatePhoneCallSignalingData).Data
		_ = client.SendSignalingData(user.ID, signalingData)
		return nil
	})
}
