package ubot

import (
	"gotgcalls/ntgcalls"

	tg "github.com/amarnathcjd/gogram/telegram"
)

func parseRTCServers(connections []tg.PhoneConnection) []ntgcalls.RTCServer {
	rtcServers := make([]ntgcalls.RTCServer, len(connections))
	for i, connection := range connections {
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
	return rtcServers
}
