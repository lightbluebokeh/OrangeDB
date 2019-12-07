package main

import (
	"fmt"
	"runtime"
	"syscall"
	"time"
	"unsafe"
)

type ExecResult struct {
	TIME    float64 `json:"time"`
	RESULTS string  `json:"results""`
}

// Exec 调用 dll 库执行语句
// 还没有测试过
func Exec(sql string) ExecResult {
	var libPath string
	if runtime.GOOS == "windows" {
		libPath = "../dbms/bin/dbms.dll"
	} else {
		libPath = "../dbms/bin/libdbms.so"
	}

	// 待优化：只加载一次
	var dbmsLib = syscall.MustLoadDLL(libPath)
	defer dbmsLib.Release()
	fmt.Println(dbmsLib.Name, "loaded")
	var exec = dbmsLib.MustFindProc("exec")

	var startTime = time.Now()
	var result, _, err = exec.Call(uintptr(unsafe.Pointer(syscall.StringBytePtr(sql))))
	var elapsed = time.Since(startTime)

	if err != nil {
		result := toGoString(result)
		return ExecResult{TIME: elapsed.Seconds(), RESULTS: result}
	}
	panic(err)
}
