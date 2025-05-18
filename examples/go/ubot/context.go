package ubot

import (
	"github.com/Laky-64/gologging"
	tg "github.com/amarnathcjd/gogram/telegram"
	"gotgcalls/ntgcalls"
	"gotgcalls/ubot/types"
	"sync"
)

type Context struct {
	binding               *ntgcalls.Client
	app                   *tg.Client
	mutedByAdmin          []int64
	presentations         []int64
	pendingPresentation   map[int64]bool
	p2pConfigs            map[int64]*types.P2PConfig
	inputCalls            map[int64]*tg.InputPhoneCall
	inputGroupCalls       map[int64]*tg.InputGroupCall
	participantsMutex     sync.Mutex
	callParticipants      map[int64]*types.CallParticipantsCache
	pendingConnections    map[int64]*types.PendingConnection
	callSources           map[int64]*types.CallSources
	waitConnect           map[int64]chan error
	self                  *tg.UserObj
	incomingCallCallbacks []func(client *Context, chatId int64)
	streamEndCallbacks    []ntgcalls.StreamEndCallback
	frameCallbacks        []ntgcalls.FrameCallback
}

func NewInstance(app *tg.Client) *Context {
	client := &Context{
		binding:             ntgcalls.NTgCalls(),
		app:                 app,
		pendingPresentation: make(map[int64]bool),
		p2pConfigs:          make(map[int64]*types.P2PConfig),
		inputCalls:          make(map[int64]*tg.InputPhoneCall),
		inputGroupCalls:     make(map[int64]*tg.InputGroupCall),
		pendingConnections:  make(map[int64]*types.PendingConnection),
		callParticipants:    make(map[int64]*types.CallParticipantsCache),
		callSources:         make(map[int64]*types.CallSources),
		waitConnect:         make(map[int64]chan error),
	}
	if app.IsConnected() {
		self, err := app.GetMe()
		if err != nil {
			gologging.Fatal(err)
		}
		client.self = self
	}
	client.handleUpdates()
	return client
}

func (ctx *Context) OnIncomingCall(callback func(client *Context, chatId int64)) {
	ctx.incomingCallCallbacks = append(ctx.incomingCallCallbacks, callback)
}

func (ctx *Context) OnStreamEnd(callback ntgcalls.StreamEndCallback) {
	ctx.streamEndCallbacks = append(ctx.streamEndCallbacks, callback)
}

func (ctx *Context) OnFrame(callback ntgcalls.FrameCallback) {
	ctx.frameCallbacks = append(ctx.frameCallbacks, callback)
}

func (ctx *Context) Close() {
	ctx.binding.Free()
}
