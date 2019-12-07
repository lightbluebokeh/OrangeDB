// 封装C的string
package main

// #include <stdlib.h>
import "C"

import (
	"unsafe"
)

func toGoString(cPtr uintptr) string {
	return C.GoString((*C.char)(unsafe.Pointer(cPtr)))
}

func free(ptr uintptr) {
	C.free(unsafe.Pointer(ptr))
}
