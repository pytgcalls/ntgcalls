package ubot

import (
	"context"
	"fmt"
	"gotgcalls/ntgcalls"
	"gotgcalls/ubot/types"
	"sync"
	"time"

	"github.com/Laky-64/gologging"
	"github.com/mtgo-labs/mtgo/telegram"
	"github.com/mtgo-labs/mtgo/tg"
)

type Context struct {
	binding               *ntgcalls.Client
	app                   *telegram.Client
	mutedByAdmin          []int64
	presentations         []int64
	pendingPresentation   map[int64]bool
	p2pConfigs            map[int64]*types.P2PConfig
	inputCalls            map[int64]*tg.InputPhoneCall
	inputGroupCalls       map[int64]tg.InputGroupCallClass
	participantsMutex     sync.Mutex
	callParticipants      map[int64]*types.CallParticipantsCache
	pendingConnections    map[int64]*types.PendingConnection
	callSources           map[int64]*types.CallSources
	waitConnect           map[int64]chan error
	self                  *tg.InputPeerUser
	incomingCallCallbacks []func(client *Context, chatId int64)
	streamEndCallbacks    []ntgcalls.StreamEndCallback
	frameCallbacks        []ntgcalls.FrameCallback
	emojisCallbacks       []ntgcalls.EmojisCallback
}

func NewInstance(app *telegram.Client) *Context {
	client := &Context{
		binding:             ntgcalls.NTgCalls(),
		app:                 app,
		pendingPresentation: make(map[int64]bool),
		p2pConfigs:          make(map[int64]*types.P2PConfig),
		inputCalls:          make(map[int64]*tg.InputPhoneCall),
		inputGroupCalls:     make(map[int64]tg.InputGroupCallClass),
		pendingConnections:  make(map[int64]*types.PendingConnection),
		callParticipants:    make(map[int64]*types.CallParticipantsCache),
		callSources:         make(map[int64]*types.CallSources),
		waitConnect:         make(map[int64]chan error),
	}
	if app.IsConnected() {
		self, err := app.GetMe(context.Background())
		if err != nil {
			gologging.Fatal(err)
		}
		client.self = &tg.InputPeerUser{
			UserID:     self.ID,
			AccessHash: self.AccessHash,
		}
	}
	client.handleUpdates()
	return client
}

func (ctx *Context) invoke(query tg.TLObject) (tg.TLObject, error) {
	return ctx.app.Invoke(
		context.Background(),
		query,
		3,
		time.Second,
	)
}

func (ctx *Context) getSendableUser(peerID int64) (tg.InputUserClass, error) {
	peer, err := ctx.app.ResolvePeer(context.Background(), peerID)
	if err != nil {
		return nil, err
	}
	switch p := peer.(type) {
	case *tg.InputPeerUser:
		return &tg.InputUser{
			UserID:     p.UserID,
			AccessHash: p.AccessHash,
		}, nil
	}
	return nil, fmt.Errorf("peer is not an input peer user")
}

func (ctx *Context) OnIncomingCall(callback func(client *Context, chatId int64)) {
	ctx.incomingCallCallbacks = append(ctx.incomingCallCallbacks, callback)
}

func (ctx *Context) OnStreamEnd(callback ntgcalls.StreamEndCallback) {
	ctx.streamEndCallbacks = append(ctx.streamEndCallbacks, callback)
}

func (ctx *Context) OnUpdateEmojis(callback ntgcalls.EmojisCallback) {
	ctx.emojisCallbacks = append(ctx.emojisCallbacks, callback)
}

func (ctx *Context) OnFrame(callback ntgcalls.FrameCallback) {
	ctx.frameCallbacks = append(ctx.frameCallbacks, callback)
}

func (ctx *Context) Close() {
	ctx.binding.Free()
}
