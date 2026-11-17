package types

type CallMessagesCache struct {
	SendingIDs              map[int64]struct{}
	MessageIDs              map[int32]struct{}
	ConferenceIDs           map[int64]int32
	ConferenceAutoIncrement int32
}
