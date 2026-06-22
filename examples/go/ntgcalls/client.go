package ntgcalls

type Client struct {
	ptr                          uintptr
	connectionChangeCallbacks    []ConnectionChangeCallback
	streamEndCallbacks           []StreamEndCallback
	upgradeCallbacks             []UpgradeCallback
	signalCallbacks              []SignalCallback
	frameCallbacks               []FrameCallback
	remoteSourceCallbacks        []RemoteSourceCallback
	broadcastTimestampCallbacks  []BroadcastTimestampCallback
	broadcastPartCallbacks       []BroadcastPartCallback
	emojisCallbacks              []EmojisCallback
	requestParticipantsCallbacks []RequestParticipantsCallback
	outboundBlockCallbacks       []OutboundBlockCallback
	subchainRequestCallbacks     []SubchainRequestCallback
}
