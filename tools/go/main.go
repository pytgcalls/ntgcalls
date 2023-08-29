package main

//#cgo CFLAGS: -I/../../ntgcalls/bindings
//#cgo LDFLAGS: -L/ -lntgcalls
//#include "capi.h"
import "C"
import "fmt"

func main() {
	C.test()
	fmt.Println("WORKS")
}
