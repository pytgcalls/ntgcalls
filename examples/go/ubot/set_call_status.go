package ubot

import (
	"gotgcalls/ntgcalls"

	"github.com/mtgo-labs/mtgo/tg"
)

func (ctx *Context) setCallStatus(call tg.InputGroupCallClass, state ntgcalls.MediaState) error {
	request := &tg.PhoneEditGroupCallParticipantRequest{
		Call:        call,
		Participant: ctx.self,
	}
	request.SetMuted(state.Muted)
	request.SetVideoPaused(state.VideoPaused)
	request.SetVideoStopped(state.VideoStopped)
	request.SetPresentationPaused(state.PresentationPaused)
	_, err := ctx.invoke(
		request,
	)
	return err
}
