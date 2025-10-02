package types

import (
	"time"

	tg "github.com/amarnathcjd/gogram/telegram"
)

type CallParticipantsCache struct {
	CallParticipants  map[int64]*tg.GroupCallParticipant
	LastMtprotoUpdate time.Time
}
