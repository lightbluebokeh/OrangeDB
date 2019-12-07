package main

import (
	"github.com/gin-gonic/gin"
	"net/http"
)

func main() {
	const dist = "../frontend/dist"
	var router = gin.Default()

	// static files
	router.Static("/", dist)

	// api
	var apiRouter = router.Group("/api")
	apiRouter.POST("/exec", func(c *gin.Context) {
		type ExecForm struct {
			SQL string `json:"sql" binding:"required"`
		}
		var form ExecForm
		c.BindJSON(&form)

		var result = Exec(form.SQL)
		c.JSON(http.StatusOK, result)
	})

	// handle history mode
	router.NoRoute(func(c *gin.Context) {
		c.File(dist + "/index.html")
	})

	router.Run()
}
