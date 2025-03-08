package ntgcalls

//#include "ntgcalls.h"
//extern void handleStream(uintptr_t ptr, int64_t chatID, ntg_stream_type_enum streamType, ntg_stream_device_enum streamDevice, void*);
//extern void handleUpgrade(uintptr_t ptr, int64_t chatID, ntg_media_state_struct state, void*);
//extern void handleConnectionChange(uintptr_t ptr, int64_t chatID, ntg_call_network_state_struct callNetworkState, void*);
//extern void handleSignal(uintptr_t ptr, int64_t chatID, uint8_t*, int, void*);
//extern void handleFrames(uintptr_t ptr, int64_t chatID, ntg_stream_mode_enum streamMode, ntg_stream_device_enum streamDevice, ntg_frame_struct* frames, int size, void*);
//extern void handleRemoteSourceChange(uintptr_t ptr, int64_t chatID, ntg_remote_source_struct remoteSource, void*);
import "C"
import (
	"unsafe"
)

var handlerEnd = make(map[uintptr][]StreamEndCallback)
var handlerUpgrade = make(map[uintptr][]UpgradeCallback)
var handlerConnectionChange = make(map[uintptr][]ConnectionChangeCallback)
var handlerSignal = make(map[uintptr][]SignalCallback)
var handlerFrame = make(map[uintptr][]FrameCallback)
var handlerRemoteSourceChange = make(map[uintptr][]RemoteSourceCallback)

func NTgCalls() *Client {
	instance := &Client{
		ptr: uintptr(C.ntg_init()),
	}
	C.ntg_on_stream_end(C.uintptr_t(instance.ptr), (C.ntg_stream_callback)(unsafe.Pointer(C.handleStream)), nil)
	C.ntg_on_upgrade(C.uintptr_t(instance.ptr), (C.ntg_upgrade_callback)(unsafe.Pointer(C.handleUpgrade)), nil)
	C.ntg_on_signaling_data(C.uintptr_t(instance.ptr), (C.ntg_signaling_callback)(unsafe.Pointer(C.handleSignal)), nil)
	C.ntg_on_connection_change(C.uintptr_t(instance.ptr), (C.ntg_connection_callback)(unsafe.Pointer(C.handleConnectionChange)), nil)
	C.ntg_on_frames(C.uintptr_t(instance.ptr), (C.ntg_frame_callback)(unsafe.Pointer(C.handleFrames)), nil)
	C.ntg_on_remote_source_change(C.uintptr_t(instance.ptr), (C.ntg_remote_source_callback)(unsafe.Pointer(C.handleRemoteSourceChange)), nil)
	return instance
}

//export handleStream
func handleStream(ptr C.uintptr_t, chatID C.int64_t, streamType C.ntg_stream_type_enum, streamDevice C.ntg_stream_device_enum, _ unsafe.Pointer) {
	goChatID := int64(chatID)
	goPtr := uintptr(ptr)
	var goStreamType StreamType
	if streamType == C.NTG_STREAM_AUDIO {
		goStreamType = AudioStream
	} else {
		goStreamType = VideoStream
	}
	if handlerEnd[goPtr] != nil {
		for _, x0 := range handlerEnd[goPtr] {
			go x0(goChatID, goStreamType, parseStreamDevice(streamDevice))
		}
	}
}

//export handleUpgrade
func handleUpgrade(ptr C.uintptr_t, chatID C.int64_t, state C.ntg_media_state_struct, _ unsafe.Pointer) {
	goChatID := int64(chatID)
	goPtr := uintptr(ptr)
	goState := MediaState{
		Muted:        bool(state.muted),
		VideoPaused:  bool(state.videoPaused),
		VideoStopped: bool(state.videoStopped),
	}
	if handlerUpgrade[goPtr] != nil {
		for _, x0 := range handlerUpgrade[goPtr] {
			go x0(goChatID, goState)
		}
	}
}

//export handleSignal
func handleSignal(ptr C.uintptr_t, chatID C.int64_t, data *C.uint8_t, size C.int, _ unsafe.Pointer) {
	goChatID := int64(chatID)
	goPtr := uintptr(ptr)
	if handlerSignal[goPtr] != nil {
		for _, x0 := range handlerSignal[goPtr] {
			go x0(goChatID, C.GoBytes(unsafe.Pointer(data), size))
		}
	}
}

//export handleConnectionChange
func handleConnectionChange(ptr C.uintptr_t, chatID C.int64_t, callNetworkState C.ntg_call_network_state_struct, _ unsafe.Pointer) {
	goChatID := int64(chatID)
	goPtr := uintptr(ptr)
	var goCallState CallNetworkState
	switch callNetworkState.kind {
	case C.NTG_KIND_NORMAL:
		goCallState.Kind = NormalConnection
	case C.NTG_KIND_PRESENTATION:
		goCallState.Kind = PresentationConnection
	}
	goCallState.State = parseConnectionState(callNetworkState.state)
	if handlerConnectionChange[goPtr] != nil {
		for _, x0 := range handlerConnectionChange[goPtr] {
			go x0(goChatID, goCallState)
		}
	}
}

//export handleFrames
func handleFrames(ptr C.uintptr_t, chatID C.int64_t, streamMode C.ntg_stream_mode_enum, streamDevice C.ntg_stream_device_enum, frames *C.ntg_frame_struct, size C.int, _ unsafe.Pointer) {
	goChatID := int64(chatID)
	goPtr := uintptr(ptr)
	var goStreamMode StreamMode
	switch streamMode {
	case C.NTG_STREAM_CAPTURE:
		goStreamMode = CaptureStream
	case C.NTG_STREAM_PLAYBACK:
		goStreamMode = PlaybackStream
	}
	rawFrames := make([]Frame, size)
	for i := 0; i < int(size); i++ {
		rawFrame := *(*C.ntg_frame_struct)(unsafe.Pointer(uintptr(unsafe.Pointer(frames)) + uintptr(i)*unsafe.Sizeof(C.ntg_frame_struct{})))
		rawFrames[i] = Frame{
			Ssrc: uint32(rawFrame.ssrc),
			Data: C.GoBytes(unsafe.Pointer(rawFrame.data), rawFrame.sizeData),
			FrameData: FrameData{
				AbsoluteCaptureTimestampMs: int64(frames.frameData.absoluteCaptureTimestampMs),
				Width:                      uint16(frames.frameData.width),
				Height:                     uint16(frames.frameData.height),
				Rotation:                   uint16(frames.frameData.rotation),
			},
		}
	}
	if handlerFrame[goPtr] != nil {
		for _, x0 := range handlerFrame[goPtr] {
			go x0(goChatID, goStreamMode, parseStreamDevice(streamDevice), rawFrames)
		}
	}
}

//export handleRemoteSourceChange
func handleRemoteSourceChange(ptr C.uintptr_t, chatID C.int64_t, remoteSource C.ntg_remote_source_struct, _ unsafe.Pointer) {
	goChatID := int64(chatID)
	goPtr := uintptr(ptr)
	var goRemoteState RemoteSourceState
	switch remoteSource.state {
	case C.NTG_REMOTE_ACTIVE:
		goRemoteState = ActiveRemoteSource
	case C.NTG_REMOTE_SUSPENDED:
		goRemoteState = SuspendedRemoteSource
	case C.NTG_REMOTE_INACTIVE:
		goRemoteState = InactiveRemoteSource
	}
	goRemoteSource := RemoteSource{
		Ssrc:   uint32(remoteSource.ssrc),
		State:  goRemoteState,
		Device: parseStreamDevice(remoteSource.device),
	}
	if handlerRemoteSourceChange[goPtr] != nil {
		for _, x0 := range handlerRemoteSourceChange[goPtr] {
			go x0(goChatID, goRemoteSource)
		}
	}
}

func (ctx *Client) OnStreamEnd(callback StreamEndCallback) {
	handlerEnd[ctx.ptr] = append(handlerEnd[ctx.ptr], callback)
}

func (ctx *Client) OnUpgrade(callback UpgradeCallback) {
	handlerUpgrade[ctx.ptr] = append(handlerUpgrade[ctx.ptr], callback)
}

func (ctx *Client) OnConnectionChange(callback ConnectionChangeCallback) {
	handlerConnectionChange[ctx.ptr] = append(handlerConnectionChange[ctx.ptr], callback)
}

func (ctx *Client) OnSignal(callback SignalCallback) {
	handlerSignal[ctx.ptr] = append(handlerSignal[ctx.ptr], callback)
}

func (ctx *Client) OnFrame(callback FrameCallback) {
	handlerFrame[ctx.ptr] = append(handlerFrame[ctx.ptr], callback)
}

func (ctx *Client) OnRemoteSourceChange(callback RemoteSourceCallback) {
	handlerRemoteSourceChange[ctx.ptr] = append(handlerRemoteSourceChange[ctx.ptr], callback)
}

func (ctx *Client) CreateCall(chatId int64, desc MediaDescription) (string, error) {
	var buffer [1024]C.char
	size := C.int(len(buffer))
	f := CreateFuture()
	C.ntg_create(C.uintptr_t(ctx.ptr), C.int64_t(chatId), desc.ParseToC(), &buffer[0], size, f.ParseToC())
	f.wait()
	return C.GoString(&buffer[0]), parseErrorCode(f)
}

func (ctx *Client) InitPresentation(chatId int64) (string, error) {
	var buffer [1024]C.char
	size := C.int(len(buffer))
	f := CreateFuture()
	C.ntg_init_presentation(C.uintptr_t(ctx.ptr), C.int64_t(chatId), &buffer[0], size, f.ParseToC())
	f.wait()
	return C.GoString(&buffer[0]), parseErrorCode(f)
}

func (ctx *Client) StopPresentation(chatId int64) error {
	f := CreateFuture()
	C.ntg_stop_presentation(C.uintptr_t(ctx.ptr), C.int64_t(chatId), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) AddIncomingVideo(chatId int64, endpoint string, ssrcGroups []SsrcGroup) (uint32, error) {
	buffer := new(C.uint32_t)
	f := CreateFuture()
	C.ntg_add_incoming_video(C.uintptr_t(ctx.ptr), C.int64_t(chatId), C.CString(endpoint), parseSsrcGroups(ssrcGroups), C.int(len(ssrcGroups)), buffer, f.ParseToC())
	f.wait()
	return uint32(*buffer), parseErrorCode(f)
}

func (ctx *Client) RemoveIncomingVideo(chatId int64, endpoint string) error {
	f := CreateFuture()
	C.ntg_remove_incoming_video(C.uintptr_t(ctx.ptr), C.int64_t(chatId), C.CString(endpoint), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) CreateP2PCall(chatId int64, desc MediaDescription) error {
	f := CreateFuture()
	C.ntg_create_p2p(C.uintptr_t(ctx.ptr), C.int64_t(chatId), desc.ParseToC(), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) InitExchange(chatId int64, dhConfig DhConfig, gAHash []byte) ([]byte, error) {
	var buffer [32]C.uint8_t
	size := C.int(len(buffer))
	gAHashC, gAHashSize := parseBytes(gAHash)
	dhConfigC := dhConfig.ParseToC()
	f := CreateFuture()
	C.ntg_init_exchange(C.uintptr_t(ctx.ptr), C.int64_t(chatId), &dhConfigC, gAHashC, gAHashSize, &buffer[0], size, f.ParseToC())
	f.wait()
	return C.GoBytes(unsafe.Pointer(&buffer[0]), size), parseErrorCode(f)
}

func (ctx *Client) ExchangeKeys(chatId int64, gAB []byte, fingerprint int64) (AuthParams, error) {
	f := CreateFuture()
	var buffer C.ntg_auth_params_struct
	gABC, gABSize := parseBytes(gAB)
	C.ntg_exchange_keys(C.uintptr_t(ctx.ptr), C.int64_t(chatId), gABC, gABSize, C.int64_t(fingerprint), &buffer, f.ParseToC())
	f.wait()
	return AuthParams{
		GAOrB:          C.GoBytes(unsafe.Pointer(buffer.g_a_or_b), buffer.sizeGAB),
		KeyFingerprint: int64(buffer.key_fingerprint),
	}, parseErrorCode(f)
}

func (ctx *Client) SkipExchange(chatId int64, encryptionKey []byte, isOutgoing bool) error {
	f := CreateFuture()
	encryptionKeyC, encryptionKeySize := parseBytes(encryptionKey)
	C.ntg_skip_exchange(C.uintptr_t(ctx.ptr), C.int64_t(chatId), encryptionKeyC, encryptionKeySize, C.bool(isOutgoing), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) ConnectP2P(chatId int64, rtcServers []RTCServer, versions []string, P2PAllowed bool) error {
	f := CreateFuture()
	versionsC, sizeVersions := parseStringVectorC(versions)
	C.ntg_connect_p2p(C.uintptr_t(ctx.ptr), C.int64_t(chatId), parseRtcServers(rtcServers), C.int(len(rtcServers)), versionsC, C.int(sizeVersions), C.bool(P2PAllowed), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) SendSignalingData(chatId int64, data []byte) error {
	f := CreateFuture()
	dataC, dataSize := parseBytes(data)
	C.ntg_send_signaling_data(C.uintptr_t(ctx.ptr), C.int64_t(chatId), dataC, dataSize, f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

//goland:noinspection GoUnusedExportedFunction
func GetProtocol() Protocol {
	var buffer C.ntg_protocol_struct
	C.ntg_get_protocol(&buffer)
	return Protocol{
		MinLayer:     int32(buffer.minLayer),
		MaxLayer:     int32(buffer.maxLayer),
		UdpP2P:       bool(buffer.udpP2P),
		UdpReflector: bool(buffer.udpReflector),
		Versions:     parseStringVector(unsafe.Pointer(buffer.libraryVersions), buffer.libraryVersionsSize),
	}
}

func (ctx *Client) Connect(chatId int64, params string, isPresentation bool) error {
	f := CreateFuture()
	C.ntg_connect(C.uintptr_t(ctx.ptr), C.int64_t(chatId), C.CString(params), C.bool(isPresentation), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) SetStreamSources(chatId int64, streamMode StreamMode, desc MediaDescription) error {
	f := CreateFuture()
	C.ntg_set_stream_sources(C.uintptr_t(ctx.ptr), C.int64_t(chatId), streamMode.ParseToC(), desc.ParseToC(), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) SendExternalFrame(chatId int64, streamDevice StreamDevice, data []byte, frameData FrameData) error {
	f := CreateFuture()
	dataC, dataSize := parseBytes(data)
	C.ntg_send_external_frame(C.uintptr_t(ctx.ptr), C.int64_t(chatId), streamDevice.ParseToC(), dataC, dataSize, frameData.ParseToC(), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) Pause(chatId int64) (bool, error) {
	f := CreateFuture()
	C.ntg_pause(C.uintptr_t(ctx.ptr), C.int64_t(chatId), f.ParseToC())
	f.wait()
	return parseBool(f)
}

func (ctx *Client) Resume(chatId int64) (bool, error) {
	f := CreateFuture()
	C.ntg_resume(C.uintptr_t(ctx.ptr), C.int64_t(chatId), f.ParseToC())
	f.wait()
	return parseBool(f)
}

func (ctx *Client) Mute(chatId int64) (bool, error) {
	f := CreateFuture()
	C.ntg_mute(C.uintptr_t(ctx.ptr), C.int64_t(chatId), f.ParseToC())
	f.wait()
	return parseBool(f)
}

func (ctx *Client) UnMute(chatId int64) (bool, error) {
	f := CreateFuture()
	C.ntg_unmute(C.uintptr_t(ctx.ptr), C.int64_t(chatId), f.ParseToC())
	f.wait()
	return parseBool(f)
}

func (ctx *Client) Stop(chatId int64) error {
	f := CreateFuture()
	C.ntg_stop(C.uintptr_t(ctx.ptr), C.int64_t(chatId), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) Time(chatId int64, streamMode StreamMode) (uint64, error) {
	f := CreateFuture()
	var buffer C.int64_t
	C.ntg_time(C.uintptr_t(ctx.ptr), C.int64_t(chatId), streamMode.ParseToC(), &buffer, f.ParseToC())
	f.wait()
	return uint64(buffer), parseErrorCode(f)
}

//goland:noinspection GoUnusedExportedFunction
func GetMediaDevices() MediaDevices {
	var buffer C.ntg_media_devices_struct
	C.ntg_get_media_devices(&buffer)
	return MediaDevices{
		Microphone: parseDeviceInfoVector(unsafe.Pointer(buffer.microphone), buffer.sizeMicrophone),
		Camera:     parseDeviceInfoVector(unsafe.Pointer(buffer.camera), buffer.sizeCamera),
		Screen:     parseDeviceInfoVector(unsafe.Pointer(buffer.screen), buffer.sizeScreen),
	}
}

func (ctx *Client) CpuUsage() (float64, error) {
	f := CreateFuture()
	var buffer C.double
	C.ntg_cpu_usage(C.uintptr_t(ctx.ptr), &buffer, f.ParseToC())
	f.wait()
	return float64(buffer), parseErrorCode(f)
}

func (ctx *Client) EnableGLibLoop(enable bool) {
	C.ntg_enable_g_lib_loop(C.bool(enable))
}

func (ctx *Client) EnableH264Encoder(enable bool) {
	C.ntg_enable_h264_encoder(C.bool(enable))
}

func (ctx *Client) Calls() map[int64]MediaStatus {
	mapReturn := make(map[int64]MediaStatus)

	f := CreateFuture()
	var callSize C.uint64_t
	_ = C.ntg_calls_count(C.uintptr_t(ctx.ptr), &callSize, f.ParseToC())
	f.wait()
	f = CreateFuture()
	if callSize == 0 {
		return mapReturn
	}
	buffer := make([]C.ntg_call_struct, callSize)
	C.ntg_calls(C.uintptr_t(ctx.ptr), &buffer[0], callSize, f.ParseToC())
	f.wait()
	for _, call := range buffer {
		mapReturn[int64(call.chatId)] = MediaStatus{
			Playback: parseStreamStatus(call.playback),
			Capture:  parseStreamStatus(call.capture),
		}
	}
	return mapReturn
}

//goland:noinspection GoUnusedExportedFunction
func Version() string {
	var buffer [20]C.char
	size := C.int(len(buffer))
	C.ntg_get_version(&buffer[0], size)
	return C.GoString(&buffer[0])
}

func (ctx *Client) Free() {
	C.ntg_destroy(C.uintptr_t(ctx.ptr))
	delete(handlerEnd, ctx.ptr)
	delete(handlerUpgrade, ctx.ptr)
	delete(handlerConnectionChange, ctx.ptr)
	delete(handlerSignal, ctx.ptr)
}
