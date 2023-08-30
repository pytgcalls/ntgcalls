package main

//#cgo LDFLAGS: -L/ -lntgcalls
import "C"
import (
	"fmt"
	"gotgcalls/ntgcalls"
)

func myGoFunction(a C.int, b C.int) C.int {
	sum := int(a) + int(b)
	return C.int(sum)
}

func main() {
	client := ntgcalls.NTgCalls()
	defer client.Free()
	res, err := client.CreateCall(12345, ntgcalls.MediaDescription{
		Encoder: "raw",
		Audio: &ntgcalls.AudioDescription{
			SampleRate:    48000,
			BitsPerSample: 16,
			ChannelCount:  2,
			Path:          "../output.pcm",
		},
	})
	fmt.Println(res, err)
	client.OnStreamEnd(func(chatId int64, streamType ntgcalls.StreamType) {
		fmt.Println(chatId)
	})
}
