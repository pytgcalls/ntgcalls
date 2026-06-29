package main

/*
// You can also change the file names to have multiple libraries on the same folder,
// but you need to change the name of the library in the LDFLAGS and the -L folder from . to the
// folder where the library is located.
#cgo linux LDFLAGS: -L . -lntgcalls -lm -lz
#cgo darwin LDFLAGS: -L . -lntgcalls -lc++ -lz -lbz2 -liconv -framework AVFoundation -framework AudioToolbox -framework CoreAudio -framework QuartzCore -framework CoreMedia -framework VideoToolbox -framework AppKit -framework Metal -framework MetalKit -framework OpenGL -framework IOSurface -framework ScreenCaptureKit

// Currently is supported only dynamically linked library on Windows due to
// https://github.com/golang/go/issues/63903
#cgo windows LDFLAGS: -L. -lntgcalls
#include "ntgcalls/ntgcalls.h"
#include "glibc_compatibility.h"
*/
import "C"
import (
	"fmt"
	"gotgcalls/ntgcalls"
	"gotgcalls/ubot"
	"log"

	"github.com/Laky-64/gologging"
	"github.com/mtgo-labs/mtgo/telegram"
	"github.com/mtgo-labs/mtgo/telegram/types"
	"github.com/mtgo-labs/storage/sqlite"
)

var urlVideoTest = "https://docs.evostream.com/sample_content/assets/sintel1m720p.mp4"

func main() {
	gologging.SetLevel(gologging.FatalLevel)
	gologging.GetLogger("ntgcalls").SetLevel(gologging.DebugLevel)
	mtProto, _ := telegram.NewClient(
		123,
		"a1b2c3d4e5f6",
		&telegram.Config{
			PhoneNumber: "YOUR_PHONE_NUMBER",
			SessionName: "ntgcalls.session",
			SavePeers:   true,
			Log: telegram.LogConfig{
				Level: telegram.DebugLevel,
			},
			Storage: sqlite.New(),
		},
	)
	if err := mtProto.Connect(0); err != nil {
		log.Fatalf("connect: %v", err)
	}
	defer mtProto.Stop()

	uBotInstance := ubot.NewInstance(mtProto)
	defer uBotInstance.Close()

	uBotInstance.OnIncomingCall(func(client *ubot.Context, chatId int64) {
		err := uBotInstance.Play(chatId, getMediaDescription(urlVideoTest))
		if err != nil {
			gologging.Fatal(err)
		}
	})
	uBotInstance.OnStreamEnd(func(chatId int64, streamType ntgcalls.StreamType, streamDevice ntgcalls.StreamDevice) {
		fmt.Println("Stream ended with chatId:", chatId, "streamType:", streamType, "streamDevice:", streamDevice)
	})
	uBotInstance.OnFrame(func(chatId int64, mode ntgcalls.StreamMode, device ntgcalls.StreamDevice, frames []ntgcalls.Frame) {
		fmt.Println("Received frames for chatId:", chatId, "mode:", mode, "device:", device)
	})
	mtProto.OnMessage(func(client *telegram.Client, message *types.Message) {
		err := uBotInstance.Play(message.ChatID, getMediaDescription(urlVideoTest))
		if err != nil {
			gologging.Error(err)
			return
		}
		_, err = message.Reply("Playing!")
		if err != nil {
			gologging.Error(err)
			return
		}
	}, telegram.Regex("^[!/.]play"))
	mtProto.OnMessage(func(client *telegram.Client, message *types.Message) {
		err := uBotInstance.Record(message.ChatID, ntgcalls.MediaDescription{
			Microphone: &ntgcalls.AudioDescription{
				MediaSource:  ntgcalls.MediaSourceExternal,
				SampleRate:   96000,
				ChannelCount: 2,
			},
		})
		if err != nil {
			gologging.Error(err)
			return
		}
		_, err = message.Reply("Recording!")
		if err != nil {
			gologging.Error(err)
			return
		}
	}, telegram.Regex("^[!/.]record"))
	mtProto.OnMessage(func(client *telegram.Client, message *types.Message) {
		err := uBotInstance.Stop(message.ChatID)
		if err != nil {
			gologging.Error(err)
			return
		}
		_, err = message.Reply("Stopped!")
		if err != nil {
			gologging.Error(err)
			return
		}
	}, telegram.Regex("^[!/.]stop"))
	mtProto.OnMessage(func(client *telegram.Client, message *types.Message) {
		paused, err := uBotInstance.Pause(message.ChatID)
		if err != nil {
			gologging.Error(err)
			return
		}
		if paused {
			_, err = message.Reply("Paused!")
			if err != nil {
				gologging.Error(err)
				return
			}
		}
	}, telegram.Regex("^[!/.]pause"))
	mtProto.OnMessage(func(client *telegram.Client, message *types.Message) {
		resumed, err := uBotInstance.Resume(message.ChatID)
		if err != nil {
			gologging.Error(err)
			return
		}
		if resumed {
			_, err = message.Reply("Resumed!")
			if err != nil {
				gologging.Error(err)
				return
			}
		}
	}, telegram.Regex("^[!/.]resume"))
	mtProto.OnMessage(func(client *telegram.Client, message *types.Message) {
		err := uBotInstance.JoinConference(message.ChatID, getMediaDescription(urlVideoTest))
		if err != nil {
			gologging.Error(err)
			return
		}
		_, err = message.Reply("Conference started!")
		if err != nil {
			gologging.Error(err)
			return
		}
	}, telegram.Regex("^[!/.]conference"))
	uBotInstance.OnUpdateEmojis(func(chatId int64, emojis string) {
		fmt.Println("Emojis updated for chatId:", chatId, "emojis:", emojis)
	})
	mtProto.Idle()
}

func getMediaDescription(url string) ntgcalls.MediaDescription {
	audioDescription := &ntgcalls.AudioDescription{
		MediaSource:  ntgcalls.MediaSourceShell,
		SampleRate:   96000,
		ChannelCount: 2,
	}
	videoDescription := &ntgcalls.VideoDescription{
		MediaSource: ntgcalls.MediaSourceShell,
		Width:       1280,
		Height:      720,
		Fps:         30,
	}

	baseFFmpeg := "ffmpeg -reconnect 1 -reconnect_at_eof 1 -reconnect_streamed 1 -reconnect_delay_max 2 -i"
	audioDescription.Input = fmt.Sprintf(
		"%s %s -f s16le -ac %d -ar %d -v quiet pipe:1",
		baseFFmpeg,
		url,
		audioDescription.ChannelCount,
		audioDescription.SampleRate,
	)
	videoDescription.Input = fmt.Sprintf(
		"%s %s -f rawvideo -r %d -pix_fmt yuv420p -vf scale=%d:%d -v quiet pipe:1",
		baseFFmpeg,
		url,
		videoDescription.Fps,
		videoDescription.Width,
		videoDescription.Height,
	)
	return ntgcalls.MediaDescription{
		Microphone: audioDescription,
		Camera:     videoDescription,
	}
}
