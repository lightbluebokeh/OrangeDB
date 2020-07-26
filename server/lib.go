package main

import (
	"time"
)

// #cgo LDFLAGS: -L ../dbms/lib -ldbms
// extern const char* exec(const char* sql);
// extern const char* info();
// extern void setup();
import "C"

func Setup() {
	C.setup()
}

// ExecResult : 执行 SQL 的响应结果
type ExecResult struct {
	Time    float64 `json:"time"`
	Results string  `json:"results"`
}

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
