package ubot

import (
	"gotgcalls/ntgcalls"

	"github.com/mtgo-labs/mtgo/tg"
)

func parseRTCServers(connections []tg.PhoneConnectionClass) []ntgcalls.RTCServer {
	rtcServers := make([]ntgcalls.RTCServer, len(connections))
	for i, connection := range connections {
		switch connection.(type) {
		case *tg.PhoneConnectionWebrtc:
			rtcServer := connection.(*tg.PhoneConnectionWebrtc)
			rtcServers[i] = ntgcalls.RTCServer{
				ID:       rtcServer.ID,
				Ipv4:     rtcServer.Ip,
				Ipv6:     rtcServer.IPv6,
				Username: rtcServer.Username,
				Password: rtcServer.Password,
				Port:     rtcServer.Port,
				Turn:     rtcServer.Turn,
				Stun:     rtcServer.Stun,
			}
		case *tg.PhoneConnection:
			phoneServer := connection.(*tg.PhoneConnection)
			rtcServers[i] = ntgcalls.RTCServer{
				ID:      phoneServer.ID,
				Ipv4:    phoneServer.Ip,
				Ipv6:    phoneServer.IPv6,
				Port:    phoneServer.Port,
				Turn:    true,
				Tcp:     phoneServer.TCP,
				PeerTag: phoneServer.PeerTag,
			}
		}
	}
	return rtcServers
}
