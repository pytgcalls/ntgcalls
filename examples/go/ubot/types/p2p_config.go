package types

import "github.com/mtgo-labs/mtgo/tg"

type P2PConfig struct {
	DhConfig       *tg.MessagesDHConfig
	PhoneCall      *tg.PhoneCall
	IsOutgoing     bool
	KeyFingerprint int64
	GAorB          []byte
	WaitData       chan error
}
