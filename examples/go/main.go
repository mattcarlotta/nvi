package main

import (
	"log"
	"os"
)

func assert(key string, ev string) {
	var val = os.Getenv(key)
	if val != ev {
		log.Fatalf("Expected the key \"%s\" value to match \"%s\". Actual value: \"%s\"", key, ev, val)
	}
}

func main() {
	assert("QUOTES", "sad\"wow\"bak")
	assert("MULTI_LINE_KEY", "ssh-rsa BBBBPl1P1AD+jk/3Lf3Dw== test@example.com")
	assert("MONGOLAB_URI", "mongodb://root:password@abcd1234.mongolab.com:12345/localhost")

	log.Print("Successfully assigned ENVs to the process!")
	os.Exit(0)
}
