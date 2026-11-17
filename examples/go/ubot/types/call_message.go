package types

type CallMessageAction int

const (
	CallMessageReceived CallMessageAction = 1 << iota
	CallMessageDeleted
)

type CallMessage struct {
	MessageID int32
	SenderID  int64
	Text      string
	HTML      string
	Date      int32
	Stars     int64
	FromAdmin bool
}
