package ntgcalls

type RemoteSource struct {
	Ssrc   uint32
	State  RemoteSourceState
	Device StreamDevice
}
