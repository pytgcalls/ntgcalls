package ntgcalls

//#include "ntgcalls.h"
//extern void handleStream(uint32_t uid, int64_t chatID, ntg_stream_type_enum streamType);
//extern void handleUpgrade(uint32_t uid, int64_t chatID, ntg_media_state_struct state);
import "C"
import (
	"fmt"
	"unsafe"
)

var handlerEnd = make(map[uint32][]StreamEndCallback)
var handlerUpgrade = make(map[uint32][]UpgradeCallback)

func NTgCalls() *Instance {
	instance := &Instance{
		uid:    uint32(C.ntg_init()),
		exists: true,
	}
	C.ntg_on_stream_end(C.uint32_t(instance.uid), (C.ntg_stream_callback)(unsafe.Pointer(C.handleStream)))
	C.ntg_on_upgrade(C.uint32_t(instance.uid), (C.ntg_upgrade_callback)(unsafe.Pointer(C.handleUpgrade)))
	return instance
}

//export handleStream
func handleStream(uid C.uint32_t, chatID C.int64_t, streamType C.ntg_stream_type_enum) {
	goChatID := int64(chatID)
	goUID := uint32(uid)
	var goStreamType StreamType
	if streamType == C.NTG_STREAM_AUDIO {
		goStreamType = AudioStream
	} else {
		goStreamType = VideoStream
	}
	if handlerEnd[goUID] != nil {
		for _, x0 := range handlerEnd[goUID] {
			go x0(goChatID, goStreamType)
		}
	}
}

//export handleUpgrade
func handleUpgrade(uid C.uint32_t, chatID C.int64_t, state C.ntg_media_state_struct) {
	goChatID := int64(chatID)
	goUID := uint32(uid)
	goState := MediaState{
		Muted:        bool(state.muted),
		VideoPaused:  bool(state.videoPaused),
		VideoStopped: bool(state.videoStopped),
	}
	if handlerUpgrade[goUID] != nil {
		for _, x0 := range handlerUpgrade[goUID] {
			go x0(goChatID, goState)
		}
	}
}

func (ctx *Instance) OnStreamEnd(callback StreamEndCallback) {
	handlerEnd[ctx.uid] = append(handlerEnd[ctx.uid], callback)
}

func (ctx *Instance) OnUpgrade(callback UpgradeCallback) {
	handlerUpgrade[ctx.uid] = append(handlerUpgrade[ctx.uid], callback)
}

func parseBool(res C.int) (bool, error) {
	return res == 0, parseErrorCode(res)
}

func parseTime(res C.int64_t) (uint64, error) {
	if res < 0 {
		return 0, parseErrorCode(C.int(res))
	}
	return uint64(res), nil
}

func parseErrorCode(errorCode C.int) error {
	pErrorCode := int16(errorCode)
	switch pErrorCode {
	case -100:
		return fmt.Errorf("connection already made")
	case -101:
		return fmt.Errorf("connection not found")
	case -200:
		return fmt.Errorf("file not found")
	case -201:
		return fmt.Errorf("encoder not found")
	case -202:
		return fmt.Errorf("ffmpeg not found")
	case -203:
		return fmt.Errorf("error while executing shell command")
	case -300:
		return fmt.Errorf("rtmp needed")
	case -301:
		return fmt.Errorf("invalid transport")
	case -302:
		return fmt.Errorf("connection failed")
	}
	if pErrorCode >= 0 {
		return nil
	} else {
		return fmt.Errorf("unknown error")
	}
}

func (ctx *Instance) CreateCall(chatId int64, desc MediaDescription) (string, error) {
	var buffer [1024]C.char
	size := C.int(len(buffer))
	res := C.ntg_get_params(C.uint32_t(ctx.uid), C.int64_t(chatId), desc.ParseToC(), &buffer[0], size)
	return C.GoString(&buffer[0]), parseErrorCode(res)
}

func (ctx *Instance) Connect(chatId int64, params string) error {
	return parseErrorCode(C.ntg_connect(C.uint32_t(ctx.uid), C.int64_t(chatId), C.CString(params)))
}

func (ctx *Instance) ChangeStream(chatId int64, desc MediaDescription) error {
	return parseErrorCode(C.ntg_change_stream(C.uint32_t(ctx.uid), C.int64_t(chatId), desc.ParseToC()))
}

func (ctx *Instance) Pause(chatId int64) (bool, error) {
	return parseBool(C.ntg_pause(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) Resume(chatId int64) (bool, error) {
	return parseBool(C.ntg_resume(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) Mute(chatId int64) (bool, error) {
	return parseBool(C.ntg_mute(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) UnMute(chatId int64) (bool, error) {
	return parseBool(C.ntg_unmute(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) Stop(chatId int64) error {
	return parseErrorCode(C.ntg_stop(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) Time(chatId int64) (uint64, error) {
	return parseTime(C.ntg_time(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) Calls() map[int64]StreamStatus {
	mapReturn := make(map[int64]StreamStatus)

	callSize := C.ntg_calls_count(C.uint32_t(ctx.uid))
	buffer := make([]C.ntg_group_call_struct, callSize)
	C.ntg_calls(C.uint32_t(ctx.uid), &buffer[0], callSize)

	for _, call := range buffer {
		var goStreamType StreamStatus
		switch call.status {
		case C.NTG_PLAYING:
			goStreamType = PlayingStream
		case C.NTG_PAUSED:
			goStreamType = PausedStream
		case C.NTG_IDLING:
			goStreamType = IdlingStream
		}
		mapReturn[int64(call.chatId)] = goStreamType
	}
	return mapReturn
}

func Version() string {
	var buffer [8]C.char
	size := C.int(len(buffer))
	C.ntg_get_version(&buffer[0], size)
	return C.GoString(&buffer[0])
}

func (ctx *Instance) Free() {
	C.ntg_destroy(C.uint32_t(ctx.uid))
	delete(handlerEnd, ctx.uid)
	ctx.exists = false
}
