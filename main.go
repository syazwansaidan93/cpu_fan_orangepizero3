package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/warthog618/go-gpiocdev"
)

const (
	chipName        = "gpiochip1"
	lineNumber      = 78
	consumer        = "cpu_temp_fan_control"
	fanOnTemp       = 56.0
	fanOffTemp      = 55.5
	pollingInterval = 5 * time.Second
	tempPath        = "/sys/class/thermal/thermal_zone2/temp"
)

// readCPUTemp reads the CPU temperature from the specified sysfs path.
// It returns the temperature in Celsius and an error if reading fails.
func readCPUTemp() (float64, error) {
	content, err := ioutil.ReadFile(tempPath)
	if err != nil {
		return 0, fmt.Errorf("could not read CPU temperature file %s: %w", tempPath, err)
	}

	tempStr := strings.TrimSpace(string(content))
	tempRaw, err := strconv.Atoi(tempStr)
	if err != nil {
		return 0, fmt.Errorf("could not parse temperature value '%s': %w", tempStr, err)
	}

	tempCelsius := float64(tempRaw) / 1000.0
	return tempCelsius, nil
}

func main() {
	// Initialize logging to stderr, similar to Python's print(..., file=sys.stderr)
	log.SetOutput(os.Stderr)
	log.SetFlags(0) // Disable timestamp and file info for cleaner error messages

	chip, err := gpiocdev.NewChip(chipName)
	if err != nil {
		log.Fatalf("ERROR: Could not open GPIO chip '%s': %v\nMake sure the chip name is correct and you have appropriate permissions (try running with 'sudo').", chipName, err)
	}
	// Ensure the chip is closed when main exits.
	defer func() {
		if chip != nil {
			chip.Close()
			// log.Println("GPIO chip closed.") // Disabled for error-only logging
		}
	}()

	line, err := chip.RequestLine(lineNumber, gpiocdev.AsOutput(0), gpiocdev.WithConsumer(consumer))
	if err != nil {
		log.Fatalf("ERROR: Could not request GPIO line '%d' on chip '%s': %v\nMake sure the GPIO line is not in use and you have appropriate permissions (try running with 'sudo').", lineNumber, chipName, err)
	}
	// Ensure the line is released and set to low (fan off) when main exits.
	defer func() {
		if line != nil {
			// Set value to 0 (LOW) to ensure the fan is off on exit.
			if err := line.SetValue(0); err != nil {
				log.Printf("Warning: Could not set GPIO line to 0 before releasing: %v", err)
			}
			line.Close() // This releases the line
			// log.Println("GPIO line released.") // Disabled for error-only logging
		}
	}()

	fanIsOn := false
	// log.Printf("Starting CPU fan control. Fan ON at %.1f°C, OFF at %.1f°C. Polling every %s.", fanOnTemp, fanOffTemp, pollingInterval) // Disabled for error-only logging

	for {
		temp, err := readCPUTemp()
		if err != nil {
			log.Printf("Error reading CPU temperature: %v", err)
			time.Sleep(pollingInterval)
			continue
		}

		if temp >= fanOnTemp && !fanIsOn {
			if err := line.SetValue(1); err != nil {
				log.Printf("Error setting GPIO line to ON: %v", err)
			} else {
				fanIsOn = true
				// log.Printf("Temperature: %.2f°C (>= %.1f°C). Turning fan ON.", temp, fanOnTemp) // Disabled for error-only logging
			}
		} else if temp <= fanOffTemp && fanIsOn {
			if err := line.SetValue(0); err != nil {
				log.Printf("Error setting GPIO line to OFF: %v", err)
			} else {
				fanIsOn = false
				// log.Printf("Temperature: %.2f°C (<= %.1f°C). Turning fan OFF.", temp, fanOffTemp) // Disabled for error-only logging
			}
		} else {
			// Log current status if no state change for clarity - Disabled for error-only logging
			// status := "OFF"
			// if fanIsOn {
			// 	status = "ON"
			// }
			// log.Printf("Temperature: %.2f°C. Fan is currently %s.", temp, status)
		}

		time.Sleep(pollingInterval)
	}
}

