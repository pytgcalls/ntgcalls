package types

import (
	tg "github.com/amarnathcjd/gogram/telegram"
	"time"
)

type CallParticipantsCache struct {
	CallParticipants  map[int64]*tg.GroupCallParticipant
	LastMtprotoUpdate time.Time
}
