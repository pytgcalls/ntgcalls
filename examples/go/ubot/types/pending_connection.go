package types

import "gotgcalls/ntgcalls"

type PendingConnection struct {
	MediaDescription ntgcalls.MediaDescription
	Payload          string
	Presentation     bool
}
