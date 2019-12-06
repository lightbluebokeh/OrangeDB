package main

import "github.com/gin-gonic/gin"

func main() {
	router := gin.Default()

	router.Static("/js", "../frontend/dist/js")
	router.Static("/img", ".../frontend/dist/img")

	router.GET("/", func(c *gin.Context) {

	})
	router.POST("/api/exec", func(c *gin.Context) {

	})

	router.Run()
}
