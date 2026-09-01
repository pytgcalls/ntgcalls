package ubot

import "github.com/mtgo-labs/mtgo/tg"

func parsePeer(peer tg.PeerClass) int64 {
	var participantId int64
	switch chatObj := peer.(type) {
	case *tg.PeerUser:
		participantId = chatObj.UserID
	case *tg.PeerChannel:
		participantId = chatObj.ChannelID
	case *tg.PeerChat:
		participantId = chatObj.ChatID
	}
	return participantId
}
