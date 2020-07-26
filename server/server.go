package main

import (
	"github.com/gin-gonic/gin"
	"net/http"
)

func main() {
	router := gin.Default()
	const dist = "../frontend/dist"

	// static files
	router.Static("/", dist)

	// api
	apiRouter := router.Group("/api")
	//lib := LoadLib() // 加载动态链接库
	//defer lib.Release()

	//exec := lib.MustFindProc("exec")
	apiRouter.POST("/exec", func(c *gin.Context) {
		type ExecForm struct {
			SQL string `json:"sql" binding:"required"`
		}
		var form ExecForm
		c.BindJSON(&form)
		result := Exec(form.SQL)
		//result := Exec(exec, form.SQL)
		c.JSON(http.StatusOK, result)
	})

	//info := lib.MustFindProc("info")
	apiRouter.POST("/info", func(c *gin.Context) {
		//result := Info(info)
		result := Info()
		c.JSON(http.StatusOK, result)
	})

	// handle vue history mode
	router.NoRoute(func(c *gin.Context) {
		c.File(dist + "/index.html")
	})

	Setup()

	router.Run()
}
