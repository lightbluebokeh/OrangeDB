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

	apiRouter.POST("/exec", func(c *gin.Context) {
		type ExecForm struct {
			SQL string `json:"sql" binding:"required"`
		}
		var form ExecForm
		c.BindJSON(&form)
		result := Exec(form.SQL)
		c.JSON(http.StatusOK, result)
	})

	apiRouter.POST("/info", func(c *gin.Context) {
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
