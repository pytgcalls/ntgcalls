package ntgcalls

import (
	"sync"
)

// #include "ntgcalls.h"
// extern void unlockMutex(void*);
import "C"
import (
	"unsafe"
)

type Future struct {
	mutex      *sync.Mutex
	errCode    *C.int
	errMessage **C.char
}

func CreateFuture() *Future {
	res := &Future{
		mutex:      &sync.Mutex{},
		errCode:    new(C.int),
		errMessage: new(*C.char),
	}
	res.mutex.Lock()
	return res
}

func (ctx *Future) ParseToC() C.ntg_async_struct {
	var x C.ntg_async_struct
	x.userData = unsafe.Pointer(ctx.mutex)
	x.promise = (C.ntg_async_callback)(unsafe.Pointer(C.unlockMutex))
	x.errorCode = (*C.int)(unsafe.Pointer(ctx.errCode))
	x.errorMessage = ctx.errMessage
	return x
}

func (ctx *Future) wait() {
	ctx.mutex.Lock()
}

//export unlockMutex
func unlockMutex(p unsafe.Pointer) {
	m := (*sync.Mutex)(p)
	m.Unlock()
}
