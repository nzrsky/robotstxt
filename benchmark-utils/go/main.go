package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"os"

	"github.com/jimsmart/grobotstxt"
)

func loadRobotsFiles(filename string) ([]string, error) {
	f, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer f.Close()

	var files []string
	for {
		var length uint32
		err := binary.Read(f, binary.LittleEndian, &length)
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, err
		}

		data := make([]byte, length)
		_, err = io.ReadFull(f, data)
		if err != nil {
			return nil, err
		}
		files = append(files, string(data))
	}
	return files, nil
}

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintln(os.Stderr, "Usage: go-bench <robots_all.bin>")
		os.Exit(1)
	}

	files, err := loadRobotsFiles(os.Args[1])
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error loading files: %v\n", err)
		os.Exit(1)
	}

	// Parse and match all files
	allowed := 0
	for _, content := range files {
		if grobotstxt.AgentAllowed(content, "Googlebot", "/") {
			allowed++
		}
	}

	fmt.Printf("Processed %d files, %d allowed\n", len(files), allowed)
}
