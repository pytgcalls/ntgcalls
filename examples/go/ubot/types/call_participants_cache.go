package types

import (
	"time"

	"github.com/mtgo-labs/mtgo/tg"
)

type CallParticipantsCache struct {
	CallParticipants  map[int64]*tg.GroupCallParticipant
	LastMTProtoUpdate time.Time
}
