package main

import (
	"runtime"
	"syscall"
	"time"
)

// ExecResult : 执行 SQL 的响应结果
type ExecResult struct {
	Time    float64 `json:"time"`
	Results string  `json:"results"`
}

// LoadLib : 从某路径加载 c++ 动态链接库
func LoadLib() *syscall.DLL {
	var libPath string
	if runtime.GOOS == "windows" {
		libPath = "../dbms/bin/dbms.dll"
	} else {
		libPath = "../dbms/bin/libdbms.so"
	}
	return syscall.MustLoadDLL(libPath)
}

// Exec : 调用 exec 函数，执行 SQL 语句
func Exec(exec *syscall.Proc, sql string) ExecResult {
	startTime := time.Now()
	result, _, err := exec.Call(toCString(sql))
	elapsed := time.Since(startTime)

	if err != nil {
		result := toGoString(result)
		return ExecResult{Time: elapsed.Seconds(), Results: result}
	}
	panic(err)
}

// InfoResult : 查询到的信息
type InfoResult struct {
	Info string `json:"info"`
}

// Info : 查询数据库的信息，显示在右边
func Info(info *syscall.Proc) InfoResult {
	result, _, err := info.Call()
	if err != nil {
		result := toGoString(result)
		return InfoResult{Info: result}
	}
	panic(err)
}
