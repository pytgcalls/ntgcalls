package ubot

import (
	"gotgcalls/ubot/types"

	"github.com/mtgo-labs/mtgo/tg"
)

func (ctx *Context) getP2PConfigs(GAorB []byte) (*types.P2PConfig, error) {
	dhConfig, err := ctx.invoke(
		&tg.MessagesGetDHConfigRequest{
			Version:      0,
			RandomLength: 256,
		},
	)
	if err != nil {
		return nil, err
	}
	return &types.P2PConfig{
		DhConfig:   dhConfig.(*tg.MessagesDHConfig),
		IsOutgoing: GAorB == nil,
		GAorB:      GAorB,
		WaitData:   make(chan error),
	}, nil
}
