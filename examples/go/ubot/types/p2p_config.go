package types

import tg "github.com/amarnathcjd/gogram/telegram"

type P2PConfig struct {
	DhConfig       *tg.MessagesDhConfigObj
	PhoneCall      *tg.PhoneCallObj
	IsOutgoing     bool
	KeyFingerprint int64
	GAorB          []byte
	WaitData       chan error
}
