package ntgcalls

//#include "ntgcalls.h"
//#include <stdlib.h>
//extern void handleStreamEnd(uintptr_t ptr, int64_t chatID, ntg_stream_type_enum streamType, ntg_stream_device_enum streamDevice, void*);
//extern void handleUpgrade(uintptr_t ptr, int64_t chatID, ntg_media_state_struct state, void*);
//extern void handleConnectionChange(uintptr_t ptr, int64_t chatID, ntg_network_info_struct networkInfo, void*);
//extern void handleSignal(uintptr_t ptr, int64_t chatID, uint8_t*, int, void*);
//extern void handleFrames(uintptr_t ptr, int64_t chatID, ntg_stream_mode_enum streamMode, ntg_stream_device_enum streamDevice, ntg_frame_struct* frames, int size, void*);
//extern void handleRemoteSourceChange(uintptr_t ptr, int64_t chatID, ntg_remote_source_struct remoteSource, void*);
//extern void handleRequestBroadcastTimestamp(uintptr_t ptr, int64_t chatID, void*);
//extern void handleRequestBroadcastPart(uintptr_t ptr, int64_t chatID, ntg_segment_part_request_struct segmentPartRequest, void*);
//extern void handleLogs(ntg_log_message_struct logMessage);
import "C"
import (
	"fmt"
	"unsafe"

	"github.com/Laky-64/gologging"
)

func init() {
	C.ntg_register_logger((C.ntg_log_message_callback)(unsafe.Pointer(C.handleLogs)))
}

func NTgCalls() *Client {
	instance := &Client{
		ptr: uintptr(C.ntg_init()),
	}
	selfPointer := unsafe.Pointer(instance)
	C.ntg_on_stream_end(C.uintptr_t(instance.ptr), (C.ntg_stream_callback)(unsafe.Pointer(C.handleStreamEnd)), selfPointer)
	C.ntg_on_upgrade(C.uintptr_t(instance.ptr), (C.ntg_upgrade_callback)(unsafe.Pointer(C.handleUpgrade)), selfPointer)
	C.ntg_on_signaling_data(C.uintptr_t(instance.ptr), (C.ntg_signaling_callback)(unsafe.Pointer(C.handleSignal)), selfPointer)
	C.ntg_on_connection_change(C.uintptr_t(instance.ptr), (C.ntg_connection_callback)(unsafe.Pointer(C.handleConnectionChange)), selfPointer)
	C.ntg_on_frames(C.uintptr_t(instance.ptr), (C.ntg_frame_callback)(unsafe.Pointer(C.handleFrames)), selfPointer)
	C.ntg_on_remote_source_change(C.uintptr_t(instance.ptr), (C.ntg_remote_source_callback)(unsafe.Pointer(C.handleRemoteSourceChange)), selfPointer)
	C.ntg_on_request_broadcast_timestamp(C.uintptr_t(instance.ptr), (C.ntg_broadcast_timestamp_callback)(unsafe.Pointer(C.handleRequestBroadcastTimestamp)), selfPointer)
	C.ntg_on_request_broadcast_part(C.uintptr_t(instance.ptr), (C.ntg_broadcast_part_callback)(unsafe.Pointer(C.handleRequestBroadcastPart)), selfPointer)
	return instance
}

//export handleLogs
func handleLogs(logMessage C.ntg_log_message_struct) {
	message := fmt.Sprintf(
		"(%s:%d) %s",
		string(C.GoString(logMessage.file)),
		uint32(logMessage.line),
		string(C.GoString(logMessage.message)),
	)
	var loggerName string
	if logMessage.source == C.NTG_LOG_WEBRTC {
		loggerName = "webrtc"
	} else {
		loggerName = "ntgcalls"
	}
	loggerInstance := gologging.GetLogger(loggerName)
	switch logMessage.level {
	case C.NTG_LOG_DEBUG:
		loggerInstance.Debug(message)
	case C.NTG_LOG_INFO:
		loggerInstance.Info(message)
	case C.NTG_LOG_WARNING:
		loggerInstance.Warn(message)
	case C.NTG_LOG_ERROR:
		loggerInstance.Error(message)
	}
}

//export handleStreamEnd
func handleStreamEnd(_ C.uintptr_t, chatID C.int64_t, streamType C.ntg_stream_type_enum, streamDevice C.ntg_stream_device_enum, ptr unsafe.Pointer) {
	self := (*Client)(ptr)
	goChatID := int64(chatID)
	var goStreamType StreamType
	if streamType == C.NTG_STREAM_AUDIO {
		goStreamType = AudioStream
	} else {
		goStreamType = VideoStream
	}
	for _, x0 := range self.streamEndCallbacks {
		go x0(goChatID, goStreamType, parseStreamDevice(streamDevice))
	}
}

//export handleUpgrade
func handleUpgrade(_ C.uintptr_t, chatID C.int64_t, state C.ntg_media_state_struct, ptr unsafe.Pointer) {
	self := (*Client)(ptr)
	goChatID := int64(chatID)
	goState := MediaState{
		Muted:              bool(state.muted),
		VideoPaused:        bool(state.videoPaused),
		VideoStopped:       bool(state.videoStopped),
		PresentationPaused: bool(state.presentationPaused),
	}
	for _, x0 := range self.upgradeCallbacks {
		go x0(goChatID, goState)
	}
}

//export handleSignal
func handleSignal(_ C.uintptr_t, chatID C.int64_t, data *C.uint8_t, size C.int, ptr unsafe.Pointer) {
	self := (*Client)(ptr)
	goChatID := int64(chatID)
	for _, x0 := range self.signalCallbacks {
		go x0(goChatID, C.GoBytes(unsafe.Pointer(data), size))
	}
}

//export handleConnectionChange
func handleConnectionChange(_ C.uintptr_t, chatID C.int64_t, networkInfo C.ntg_network_info_struct, ptr unsafe.Pointer) {
	self := (*Client)(ptr)
	goChatID := int64(chatID)
	var goCallState NetworkInfo
	switch networkInfo.kind {
	case C.NTG_KIND_NORMAL:
		goCallState.Kind = NormalConnection
	case C.NTG_KIND_PRESENTATION:
		goCallState.Kind = PresentationConnection
	}
	goCallState.State = parseConnectionState(networkInfo.state)
	for _, x0 := range self.connectionChangeCallbacks {
		go x0(goChatID, goCallState)
	}
}

//export handleFrames
func handleFrames(_ C.uintptr_t, chatID C.int64_t, streamMode C.ntg_stream_mode_enum, streamDevice C.ntg_stream_device_enum, frames *C.ntg_frame_struct, size C.int, ptr unsafe.Pointer) {
	self := (*Client)(ptr)
	goChatID := int64(chatID)
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
	for _, x0 := range self.frameCallbacks {
		go x0(goChatID, goStreamMode, parseStreamDevice(streamDevice), rawFrames)
	}
}

//export handleRemoteSourceChange
func handleRemoteSourceChange(_ C.uintptr_t, chatID C.int64_t, remoteSource C.ntg_remote_source_struct, ptr unsafe.Pointer) {
	self := (*Client)(ptr)
	goChatID := int64(chatID)
	goRemoteSource := RemoteSource{
		Ssrc:   uint32(remoteSource.ssrc),
		State:  parseStreamStatus(remoteSource.state),
		Device: parseStreamDevice(remoteSource.device),
	}
	for _, x0 := range self.remoteSourceCallbacks {
		go x0(goChatID, goRemoteSource)
	}
}

//export handleRequestBroadcastTimestamp
func handleRequestBroadcastTimestamp(_ C.uintptr_t, chatID C.int64_t, ptr unsafe.Pointer) {
	self := (*Client)(ptr)
	goChatID := int64(chatID)
	for _, x0 := range self.broadcastTimestampCallbacks {
		go x0(goChatID)
	}
}

//export handleRequestBroadcastPart
func handleRequestBroadcastPart(_ C.uintptr_t, chatID C.int64_t, segmentPartRequest C.ntg_segment_part_request_struct, ptr unsafe.Pointer) {
	self := (*Client)(ptr)
	goChatID := int64(chatID)
	var goSegmentQuality MediaSegmentQuality
	switch segmentPartRequest.quality {
	case C.NTG_MEDIA_SEGMENT_QUALITY_NONE:
		goSegmentQuality = SegmentQualityNone
	case C.NTG_MEDIA_SEGMENT_QUALITY_THUMBNAIL:
		goSegmentQuality = SegmentQualityThumbnail
	case C.NTG_MEDIA_SEGMENT_QUALITY_MEDIUM:
		goSegmentQuality = SegmentQualityMedium
	case C.NTG_MEDIA_SEGMENT_QUALITY_FULL:
		goSegmentQuality = SegmentQualityFull
	}
	goSegmentPartRequest := SegmentPartRequest{
		SegmentID:     int64(segmentPartRequest.segmentId),
		PartID:        int32(segmentPartRequest.partId),
		Limit:         int32(segmentPartRequest.limit),
		Timestamp:     int64(segmentPartRequest.timestamp),
		QualityUpdate: bool(segmentPartRequest.qualityUpdate),
		ChannelID:     int32(segmentPartRequest.channelId),
		Quality:       goSegmentQuality,
	}
	for _, x0 := range self.broadcastPartCallbacks {
		go x0(goChatID, goSegmentPartRequest)
	}
}

func (ctx *Client) OnStreamEnd(callback StreamEndCallback) {
	ctx.streamEndCallbacks = append(ctx.streamEndCallbacks, callback)
}

func (ctx *Client) OnUpgrade(callback UpgradeCallback) {
	ctx.upgradeCallbacks = append(ctx.upgradeCallbacks, callback)
}

func (ctx *Client) OnConnectionChange(callback ConnectionChangeCallback) {
	ctx.connectionChangeCallbacks = append(ctx.connectionChangeCallbacks, callback)
}

func (ctx *Client) OnSignal(callback SignalCallback) {
	ctx.signalCallbacks = append(ctx.signalCallbacks, callback)
}

func (ctx *Client) OnFrame(callback FrameCallback) {
	ctx.frameCallbacks = append(ctx.frameCallbacks, callback)
}

func (ctx *Client) OnRemoteSourceChange(callback RemoteSourceCallback) {
	ctx.remoteSourceCallbacks = append(ctx.remoteSourceCallbacks, callback)
}

func (ctx *Client) OnRequestBroadcastTimestamp(callback BroadcastTimestampCallback) {
	ctx.broadcastTimestampCallbacks = append(ctx.broadcastTimestampCallbacks, callback)
}

func (ctx *Client) OnRequestBroadcastPart(callback BroadcastPartCallback) {
	ctx.broadcastPartCallbacks = append(ctx.broadcastPartCallbacks, callback)
}

func (ctx *Client) GetState(chatId int64) (MediaState, error) {
	f := CreateFuture()
	var buffer C.ntg_media_state_struct
	C.ntg_get_state(C.uintptr_t(ctx.ptr), C.int64_t(chatId), &buffer, f.ParseToC())
	f.wait()
	err := parseErrorCode(f)
	if err != nil {
		return MediaState{}, err
	}
	return MediaState{
		Muted:              bool(buffer.muted),
		VideoPaused:        bool(buffer.videoPaused),
		VideoStopped:       bool(buffer.videoStopped),
		PresentationPaused: bool(buffer.presentationPaused),
	}, nil
}

func (ctx *Client) GetConnectionMode(chatId int64) (ConnectionMode, error) {
	f := CreateFuture()
	var buffer C.ntg_connection_mode_enum
	C.ntg_get_connection_mode(C.uintptr_t(ctx.ptr), C.int64_t(chatId), &buffer, f.ParseToC())
	f.wait()
	err := parseErrorCode(f)
	if err != nil {
		return ConnectionMode(0), err
	}
	switch buffer {
	case C.NTG_CONNECTION_MODE_RTC:
		return RtcConnection, nil
	case C.NTG_CONNECTION_MODE_STREAM:
		return StreamConnection, nil
	case C.NTG_CONNECTION_MODE_RTMP:
		return RTMPConnection, nil
	default:
		return ConnectionMode(0), fmt.Errorf("unknown connection mode")
	}
}

func (ctx *Client) CreateCall(chatId int64) (string, error) {
	var buffer *C.char
	f := CreateFuture()
	C.ntg_create(C.uintptr_t(ctx.ptr), C.int64_t(chatId), &buffer, f.ParseToC())
	f.wait()
	defer C.free(unsafe.Pointer(buffer))
	return C.GoString(buffer), parseErrorCode(f)
}

func (ctx *Client) InitPresentation(chatId int64) (string, error) {
	var buffer *C.char
	f := CreateFuture()
	C.ntg_init_presentation(C.uintptr_t(ctx.ptr), C.int64_t(chatId), &buffer, f.ParseToC())
	f.wait()
	defer C.free(unsafe.Pointer(buffer))
	return C.GoString(buffer), parseErrorCode(f)
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

func (ctx *Client) CreateP2PCall(chatId int64) error {
	f := CreateFuture()
	C.ntg_create_p2p(C.uintptr_t(ctx.ptr), C.int64_t(chatId), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) InitExchange(chatId int64, dhConfig DhConfig, gAHash []byte) ([]byte, error) {
	var buffer *C.uint8_t
	var size C.int
	gAHashC, gAHashSize := parseBytes(gAHash)
	dhConfigC := dhConfig.ParseToC()
	f := CreateFuture()
	C.ntg_init_exchange(C.uintptr_t(ctx.ptr), C.int64_t(chatId), &dhConfigC, gAHashC, gAHashSize, &buffer, &size, f.ParseToC())
	f.wait()
	defer C.free(unsafe.Pointer(buffer))
	return C.GoBytes(unsafe.Pointer(buffer), size), parseErrorCode(f)
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

func (ctx *Client) SendBroadcastTimestamp(chatId int64, timestamp int64) error {
	f := CreateFuture()
	C.ntg_send_broadcast_timestamp(C.uintptr_t(ctx.ptr), C.int64_t(chatId), C.int64_t(timestamp), f.ParseToC())
	f.wait()
	return parseErrorCode(f)
}

func (ctx *Client) SendBroadcastPart(chatId int64, segmentID int64, partID int32, status MediaSegmentStatus, qualityUpdate bool, data []byte) error {
	f := CreateFuture()
	dataC, dataSize := parseBytes(data)
	C.ntg_send_broadcast_part(C.uintptr_t(ctx.ptr), C.int64_t(chatId), C.int64_t(segmentID), C.int32_t(partID), status.ParseToC(), C.bool(qualityUpdate), dataC, dataSize, f.ParseToC())
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
		Speaker:    parseDeviceInfoVector(unsafe.Pointer(buffer.speaker), buffer.sizeSpeaker),
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

func (ctx *Client) Calls() map[int64]*CallInfo {
	mapReturn := make(map[int64]*CallInfo)
	f := CreateFuture()
	var buffer *C.ntg_call_info_struct
	var size C.int
	C.ntg_calls(C.uintptr_t(ctx.ptr), &buffer, &size, f.ParseToC())
	f.wait()
	for i := 0; i < int(size); i++ {
		rawCall := *(*C.ntg_call_info_struct)(unsafe.Pointer(uintptr(unsafe.Pointer(buffer)) + uintptr(i)*unsafe.Sizeof(C.ntg_call_info_struct{})))
		mapReturn[int64(rawCall.chatId)] = &CallInfo{
			Playback: parseStreamStatus(rawCall.playback),
			Capture:  parseStreamStatus(rawCall.capture),
		}
	}
	defer C.free(unsafe.Pointer(buffer))
	return mapReturn
}

//goland:noinspection GoUnusedExportedFunction
func Version() string {
	var buffer *C.char
	C.ntg_get_version(&buffer)
	defer C.free(unsafe.Pointer(buffer))
	return C.GoString(buffer)
}

func (ctx *Client) Free() {
	C.ntg_destroy(C.uintptr_t(ctx.ptr))
}
