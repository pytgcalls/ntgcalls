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
	C.OnStreamEnd(C.uint32_t(instance.uid), (C.StreamEndCallback)(unsafe.Pointer(C.handleStream)))
	C.OnUpgrade(C.uint32_t(instance.uid), (C.UpgradeCallback)(unsafe.Pointer(C.handleUpgrade)))
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

func parseErrorCode(errorCode C.int) error {
	pErrorCode := int8(errorCode)
	switch pErrorCode {
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
	res := C.CreateCall(C.uint32_t(ctx.uid), C.int64_t(chatId), desc.ParseToC(), &buffer[0], size)
	return C.GoString(&buffer[0]), parseErrorCode(res)
}

func (ctx *Instance) Connect(chatId int64, params string) error {
	return parseErrorCode(C.ConnectCall(C.uint32_t(ctx.uid), C.int64_t(chatId), C.CString(params)))
}

func (ctx *Instance) ChangeStream(chatId int64, desc MediaDescription) error {
	return parseErrorCode(C.ChangeStream(C.uint32_t(ctx.uid), C.int64_t(chatId), desc.ParseToC()))
}

func (ctx *Instance) Pause(chatId int64) bool {
	return bool(C.Pause(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) Resume(chatId int64) bool {
	return bool(C.Resume(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) Mute(chatId int64) bool {
	return bool(C.Mute(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) UnMute(chatId int64) bool {
	return bool(C.UnMute(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) Stop(chatId int64) error {
	return parseErrorCode(C.Stop(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) Time(chatId int64) uint64 {
	return uint64(C.Time(C.uint32_t(ctx.uid), C.int64_t(chatId)))
}

func (ctx *Instance) Calls() map[int64]StreamStatus {
	mapReturn := make(map[int64]StreamStatus)

	callSize := C.CallsCount(C.uint32_t(ctx.uid))
	buffer := make([]C.GroupCall, callSize)
	C.Calls(C.uint32_t(ctx.uid), &buffer[0], callSize)

	for _, call := range buffer {
		var goStreamType StreamStatus
		switch call.status {
		case C.Playing:
			goStreamType = PlayingStream
		case C.Paused:
			goStreamType = PausedStream
		case C.Idling:
			goStreamType = IdlingStream
		}
		mapReturn[int64(call.chatId)] = goStreamType
	}
	return mapReturn
}

func (ctx *Instance) Free() {
	C.DestroyNTgCalls(C.uint32_t(ctx.uid))
	delete(handlerEnd, ctx.uid)
	ctx.exists = false
}
