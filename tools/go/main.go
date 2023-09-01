package main

//#cgo LDFLAGS: -L . -lntgcalls
import "C"
import (
	"fmt"
	"gotgcalls/ntgcalls"
)

func main() {
	client := ntgcalls.NTgCalls()
	defer client.Free()
	res, err := client.CreateCall(12345, ntgcalls.MediaDescription{
		Audio: &ntgcalls.AudioDescription{
			InputMode:     ntgcalls.InputModeFile,
			SampleRate:    48000,
			BitsPerSample: 16,
			ChannelCount:  2,
			Input:         "../output.pcm",
		},
	})
	fmt.Println(res, err)
	client.OnStreamEnd(func(chatId int64, streamType ntgcalls.StreamType) {
		fmt.Println(chatId)
	})
}
