package main

import (
	"time"
)

// #cgo LDFLAGS: -L ../dbms/lib -ldbms
// extern const char* exec(const char* sql);
// const char* info();
import "C"

// ExecResult : 执行 SQL 的响应结果
type ExecResult struct {
	Time    float64 `json:"time"`
	Results string  `json:"results"`
}

//// LoadLib : 从某路径加载 c++ 动态链接库
//func LoadLib() *syscall.DLL {
//	var libPath string
//	if runtime.GOOS == "windows" {
//		libPath = "../dbms/lib/dbms.dll"
//	} else {
//		libPath = "../dbms/lib/libdbms.so"
//	}
//	return syscall.MustLoadDLL(libPath)
//}



// Exec : 调用 exec 函数，执行 SQL 语句
func Exec(sql string) ExecResult {
	startTime := time.Now()
	result := C.GoString(C.exec(C.CString(sql)))
	elapsed := time.Since(startTime)
	return ExecResult{Time: elapsed.Seconds(), Results: result}
}

// InfoResult : 查询到的信息
type InfoResult struct {
	Info string `json:"info"`
}

// Info : 查询数据库的信息，显示在右边
func Info() InfoResult {
	result := C.GoString(C.info())
	return InfoResult{Info: result}
}
