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
	"github.com/Laky-64/gologging"
	tg "github.com/amarnathcjd/gogram/telegram"
	"gotgcalls/ntgcalls"
	"gotgcalls/ubot"
)

var urlVideoTest = "https://docs.evostream.com/sample_content/assets/sintel1m720p.mp4"

func main() {
	gologging.SetLevel(gologging.FatalLevel)
	gologging.GetLogger("ntgcalls").SetLevel(gologging.DebugLevel)
	mtproto, _ := tg.NewClient(tg.ClientConfig{
		AppID:   10029733,
		AppHash: "d0d81009d46e774f78c0e0e622f5fa21",
		Session: "session",
	})
	_ = mtproto.Start()

	uBotInstance := ubot.NewInstance(mtproto)
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
	mtproto.On("message:[!/.]play", func(message *tg.NewMessage) error {
		err := uBotInstance.Play(message.ChannelID(), getMediaDescription(urlVideoTest))
		if err != nil {
			return err
		}
		_, err = message.Reply("Playing!")
		if err != nil {
			return err
		}
		return nil
	})
	mtproto.On("message:[!/.]record", func(message *tg.NewMessage) error {
		err := uBotInstance.Record(message.ChannelID(), ntgcalls.MediaDescription{
			Microphone: &ntgcalls.AudioDescription{
				MediaSource:  ntgcalls.MediaSourceExternal,
				SampleRate:   96000,
				ChannelCount: 2,
			},
		})
		if err != nil {
			return err
		}
		_, err = message.Reply("Recording!")
		if err != nil {
			return err
		}
		return nil
	})
	mtproto.On("message:[!/.]stop", func(message *tg.NewMessage) error {
		err := uBotInstance.Stop(message.ChannelID())
		if err != nil {
			return err
		}
		_, err = message.Reply("Stopped!")
		if err != nil {
			return err
		}
		return nil
	})
	mtproto.On("message:[!/.]pause", func(message *tg.NewMessage) error {
		paused, err := uBotInstance.Pause(message.ChannelID())
		if err != nil {
			return err
		}
		if paused {
			_, err = message.Reply("Paused!")
			if err != nil {
				return err
			}
		}
		return nil
	})
	mtproto.On("message:[!/.]resume", func(message *tg.NewMessage) error {
		resumed, err := uBotInstance.Resume(message.ChannelID())
		if err != nil {
			return err
		}
		if resumed {
			_, err = message.Reply("Resumed!")
			if err != nil {
				return err
			}
		}
		return nil
	})
	mtproto.Idle()
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
