package ntgcalls

// #include <stdlib.h>
// #include "ntgcalls.h"
import "C"

func (ctx MediaSource) ParseToC() C.ntg_media_source_enum {
	switch ctx {
	case MediaSourceFile:
		return C.NTG_FILE
	case MediaSourceShell:
		return C.NTG_SHELL
	case MediaSourceFFmpeg:
		return C.NTG_FFMPEG
	case MediaSourceDevice:
		return C.NTG_DEVICE
	case MediaSourceDesktop:
		return C.NTG_DESKTOP
	case MediaSourceExternal:
		return C.NTG_EXTERNAL
	default:
		return C.NTG_FILE
	}
}

func (ctx StreamMode) ParseToC() C.ntg_stream_mode_enum {
	switch ctx {
	case CaptureStream:
		return C.NTG_STREAM_CAPTURE
	case PlaybackStream:
		return C.NTG_STREAM_PLAYBACK
	default:
		return C.NTG_STREAM_CAPTURE
	}
}

func (ctx StreamDevice) ParseToC() C.ntg_stream_device_enum {
	switch ctx {
	case MicrophoneStream:
		return C.NTG_STREAM_MICROPHONE
	case SpeakerStream:
		return C.NTG_STREAM_SPEAKER
	case CameraStream:
		return C.NTG_STREAM_CAMERA
	case ScreenStream:
		return C.NTG_STREAM_SCREEN
	default:
		return C.NTG_STREAM_MICROPHONE
	}
}

func (ctx MediaSegmentStatus) ParseToC() C.ntg_media_segment_status_enum {
	switch ctx {
	case SegmentStatusNotReady:
		return C.NTG_MEDIA_SEGMENT_NOT_READY
	case SegmentStatusResyncNeeded:
		return C.NTG_MEDIA_SEGMENT_RESYNC_NEEDED
	case SegmentStatusSuccess:
		return C.NTG_MEDIA_SEGMENT_SUCCESS
	default:
		return C.NTG_MEDIA_SEGMENT_NOT_READY
	}
}

func (ctx *AudioDescription) ParseToC() C.ntg_audio_description_struct {
	var x C.ntg_audio_description_struct
	x.mediaSource = ctx.MediaSource.ParseToC()
	x.input = C.CString(ctx.Input)
	x.sampleRate = C.uint32_t(ctx.SampleRate)
	x.channelCount = C.uint8_t(ctx.ChannelCount)
	x.keepOpen = C.bool(ctx.KeepOpen)
	return x
}

func (ctx *VideoDescription) ParseToC() C.ntg_video_description_struct {
	var x C.ntg_video_description_struct
	x.mediaSource = ctx.MediaSource.ParseToC()
	x.input = C.CString(ctx.Input)
	x.width = C.int16_t(ctx.Width)
	x.height = C.int16_t(ctx.Height)
	x.fps = C.uint8_t(ctx.Fps)
	x.keepOpen = C.bool(ctx.KeepOpen)
	return x
}

func (ctx *MediaDescription) ParseToC() C.ntg_media_description_struct {
	var x C.ntg_media_description_struct
	if ctx.Microphone != nil {
		microphone := ctx.Microphone.ParseToC()
		x.microphone = &microphone
	}
	if ctx.Speaker != nil {
		speaker := ctx.Speaker.ParseToC()
		x.speaker = &speaker
	}
	if ctx.Camera != nil {
		camera := ctx.Camera.ParseToC()
		x.camera = &camera
	}
	if ctx.Screen != nil {
		screen := ctx.Screen.ParseToC()
		x.screen = &screen
	}
	return x
}

func (ctx *DhConfig) ParseToC() C.ntg_dh_config_struct {
	var x C.ntg_dh_config_struct
	x.g = C.int32_t(ctx.G)
	pC, pSize := parseBytes(ctx.P)
	rC, rSize := parseBytes(ctx.Random)
	x.p = pC
	x.sizeP = pSize
	x.random = rC
	x.sizeRandom = rSize
	return x
}

func (ctx *FrameData) ParseToC() C.ntg_frame_data_struct {
	var x C.ntg_frame_data_struct
	x.absoluteCaptureTimestampMs = C.int64_t(ctx.AbsoluteCaptureTimestampMs)
	x.width = C.uint16_t(ctx.Width)
	x.height = C.uint16_t(ctx.Height)
	x.rotation = C.uint16_t(ctx.Rotation)
	return x
}
