package ubot

import (
	"gotgcalls/ubot/types"

	tg "github.com/amarnathcjd/gogram/telegram"
)

func (ctx *Context) getP2PConfigs(GAorB []byte) (*types.P2PConfig, error) {
	dhConfigRaw, err := ctx.app.MessagesGetDhConfig(0, 256)
	if err != nil {
		return nil, err
	}
	dhConfig := dhConfigRaw.(*tg.MessagesDhConfigObj)
	return &types.P2PConfig{
		DhConfig:   dhConfig,
		IsOutgoing: GAorB == nil,
		GAorB:      GAorB,
		WaitData:   make(chan error),
	}, nil
}
