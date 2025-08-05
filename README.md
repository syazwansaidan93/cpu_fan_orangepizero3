# CPU Fan Control for Orange Pi Zero 3

This repository contains a Go language script to control a CPU cooling fan on an Orange Pi Zero 3 based on the CPU temperature. It leverages the `go-gpiocdev` library to interact with the GPIO pins.

## Table of Contents

* [Features](#features)
* [Prerequisites](#prerequisites)
* [Installation](#installation)
* [Usage](#usage)
  * [Manual Execution](#manual-execution)
  * [Running as a Systemd Service](#running-as-a-systemd-service)
* [Configuration](#configuration)
* [Troubleshooting](#troubleshooting)
* [License](#license)

## Features

* Monitors CPU temperature from `/sys/class/thermal/thermal_zone2/temp`.
* Automatically turns a fan ON when the temperature reaches a specified threshold (`FAN_ON_TEMP`).
* Automatically turns the fan OFF when the temperature drops below another specified threshold (`FAN_OFF_TEMP`).
* Uses a configurable polling interval for temperature checks.
* Designed for low resource consumption, suitable for embedded systems like the Orange Pi Zero 3.
* Graceful shutdown, ensuring the fan is turned off when the script exits.

## Prerequisites

Before you begin, ensure you have the following on your Orange Pi Zero 3:

* **Go Language:** Version 1.16 or newer is recommended.
* **`libgpiod`:** The underlying C library for GPIO access. It's usually pre-installed or can be installed via your distribution's package manager (e.g., `sudo apt install libgpiod-dev`).
* **GPIO Hardware Setup:** A fan connected to GPIO pin 78 on `gpiochip1`. Please verify your specific Orange Pi Zero 3's pinout and GPIO chip/line mapping.

## Installation

Follow these steps to set up and compile the fan control script.

1.  **Navigate to your working directory:**
    ```bash
    cd /home/wan/
    ```

2.  **Create the project directory and initialize a Go module:**
    ```bash
    mkdir -p cpu_fan_control_go
    cd cpu_fan_control_go
    go mod init cpu_fan_control_go
    ```

3.  **Create the `main.go` file:**
    Save the Go code (from the Canvas document) into `main.go` inside the `cpu_fan_control_go` directory.

4.  **Download the `go-gpiocdev` dependency:**
    ```bash
    go get [github.com/warthog618/go-gpiocdev](https://github.com/warthog618/go-gpiocdev)
    ```

5.  **Build the executable:**
    ```bash
    go build -o fan_control
    ```
    This will create an executable named `fan_control` in the `cpu_fan_control_go` directory.

## Usage

### Manual Execution

You can run the script manually from your terminal. This is useful for testing.

```bash
sudo /home/wan/cpu_fan_control_go/fan_control
```
Press `Ctrl+C` to stop the script.

### Running as a Systemd Service

For automatic startup on boot and robust background operation, it's recommended to run the script as a systemd service.

1.  **Create the systemd service file:**
    ```bash
    sudo nano /etc/systemd/system/cpu-fan-control.service
    ```

2.  **Add the following content to `cpu-fan-control.service`:**
    ```ini
    [Unit]
    Description=CPU Fan Control Service
    After=network.target

    [Service]
    Type=simple
    ExecStart=/home/wan/cpu_fan_control_go/fan_control
    WorkingDirectory=/home/wan/cpu_fan_control_go
    Restart=on-failure
    RestartSec=5s
    User=root
    Group=root
    StandardOutput=journal
    StandardError=journal

    [Install]
    WantedBy=multi-user.target
    ```

3.  **Save and close the file.**

4.  **Reload the systemd daemon:**
    ```bash
    sudo systemctl daemon-reload
    ```

5.  **Enable the service to start on boot:**
    ```bash
    sudo systemctl enable cpu-fan-control.service
    ```

6.  **Start the service immediately:**
    ```bash
    sudo systemctl start cpu-fan-control.service
    ```

7.  **Check the service status and logs:**
    ```bash
    systemctl status cpu-fan-control.service
    journalctl -u cpu-fan-control.service -f
    ```

## Configuration

You can modify the following constants in the `main.go` file to adjust the fan behavior:

* `chipName`: The name of your GPIO chip (e.g., `gpiochip1`).
* `lineNumber`: The specific GPIO line number connected to your fan (e.g., `78`).
* `fanOnTemp`: The CPU temperature (in Celsius) at which the fan will turn ON (e.g., `51.0`).
* `fanOffTemp`: The CPU temperature (in Celsius) at which the fan will turn OFF (e.g., `50.5`).
* `pollingInterval`: How often the CPU temperature is checked (e.g., `5 * time.Second`).
* `tempPath`: The path to the CPU temperature file (e.g., `/sys/class/thermal/thermal_zone2/temp`).

Remember to recompile the script (`go build -o fan_control`) after making any changes to `main.go`.

## Troubleshooting

* **`ERROR: Could not open GPIO chip... Permission denied` or `Could not request GPIO line... Permission denied`:**
    * Ensure you are running the script with `sudo` or that the systemd service is configured to run as `root`.
    * Verify that the GPIO chip and line number are correct for your Orange Pi Zero 3.
    * Check if another process is already using the GPIO line.

* **`Error: CPU temperature file not found at /sys/class/thermal/thermal_zone2/temp`:**
    * The path to your CPU temperature sensor might be different. Check `/sys/class/thermal/` on your device to find the correct `thermal_zone` and update the `tempPath` constant in `main.go`.

* **Fan not turning on/off:**
    * Double-check your `FAN_ON_TEMP` and `FAN_OFF_TEMP` values.
    * Verify your fan's wiring to the specified GPIO pin.
    * Ensure the GPIO line is correctly configured as an output.

## License

This project is open-source and available under the [MIT License](LICENSE).
