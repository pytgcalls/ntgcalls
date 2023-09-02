package ntgcalls

//#include "ntgcalls.h"
//extern void handleStream(uint32_t uid, int64_t chatID, enum StreamType streamType);
//extern void handleUpgrade(uint32_t uid, int64_t chatID, MediaState state);
import "C"
import (
	"fmt"
	"unsafe"
)

var handlerEnd = make(map[uint32][]StreamEndCallback)
var handlerUpgrade = make(map[uint32][]UpgradeCallback)

func NTgCalls() *Instance {
	instance := &Instance{
		uid:    uint32(C.CreateNTgCalls()),
		exists: true,
	}
	C.OnStreamEnd(C.uint32_t(instance.uid), (C.StreamEndCallback)(unsafe.Pointer(C.handleStream)), nil)
	C.OnUpgrade(C.uint32_t(instance.uid), (C.UpgradeCallback)(unsafe.Pointer(C.handleUpgrade)), nil)
	return instance
}

//export handleStream
func handleStream(uid C.uint32_t, chatID C.int64_t, streamType C.enum_StreamType) {
	goChatID := int64(chatID)
	goUID := uint32(uid)
	var goStreamType StreamType
	if streamType == C.Audio {
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
func handleUpgrade(uid C.uint32_t, chatID C.int64_t, state C.MediaState) {
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

func parseErrorCode(errorCode int8) error {
	switch errorCode {
	case 0:
		return nil
	case -3:
		return fmt.Errorf("connection already made")
	case -4:
		return fmt.Errorf("file not found")
	case -5:
		return fmt.Errorf("encoder not found")
	case -6:
		return fmt.Errorf("ffmpeg not found")
	case -7:
		return fmt.Errorf("rtmp needed")
	case -8:
		return fmt.Errorf("invalid transport")
	case -9:
		return fmt.Errorf("connection failed")
	case -10:
		return fmt.Errorf("connection not found")
	case -11:
		return fmt.Errorf("error while executing shell command")
	default:
		return fmt.Errorf("unknown error")
	}
}

func (ctx *Instance) CreateCall(chatId int64, desc MediaDescription) (string, error) {
	var errorCode C.int8_t
	res := C.GoString(C.CreateCall(C.uint32_t(ctx.uid), C.int64_t(chatId), desc.ParseToC(), &errorCode))
	return res, parseErrorCode(int8(errorCode))
}

func (ctx *Instance) Connect(chatId int64, params string) error {
	var errorCode C.int8_t
	C.ConnectCall(C.uint32_t(ctx.uid), C.int64_t(chatId), C.CString(params), &errorCode)
	return parseErrorCode(int8(errorCode))
}

func (ctx *Instance) ChangeStream(chatId int64, desc MediaDescription) error {
	var errorCode C.int8_t
	C.ChangeStream(C.uint32_t(ctx.uid), C.int64_t(chatId), desc.ParseToC(), &errorCode)
	return parseErrorCode(int8(errorCode))
}

func (ctx *Instance) Pause(chatId int64) bool {
	return bool(C.Pause(C.uint32_t(ctx.uid), C.int64_t(chatId), nil))
}

func (ctx *Instance) Resume(chatId int64) bool {
	return bool(C.Resume(C.uint32_t(ctx.uid), C.int64_t(chatId), nil))
}

func (ctx *Instance) Mute(chatId int64) bool {
	return bool(C.Mute(C.uint32_t(ctx.uid), C.int64_t(chatId), nil))
}

func (ctx *Instance) UnMute(chatId int64) bool {
	return bool(C.UnMute(C.uint32_t(ctx.uid), C.int64_t(chatId), nil))
}

func (ctx *Instance) Stop(chatId int64) error {
	var errorCode C.int8_t
	C.UnMute(C.uint32_t(ctx.uid), C.int64_t(chatId), &errorCode)
	return parseErrorCode(int8(errorCode))
}

func (ctx *Instance) Time(chatId int64) uint64 {
	return uint64(C.Time(C.uint32_t(ctx.uid), C.int64_t(chatId), nil))
}

func (ctx *Instance) Free() {
	C.DestroyNTgCalls(C.uint32_t(ctx.uid), nil)
	delete(handlerEnd, ctx.uid)
	ctx.exists = false
}
