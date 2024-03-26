package ntgcalls

type Client struct {
	uid       uint32
	exists    bool
	streamEnd []StreamEndCallback
}
