# CPU Fan Control for Orange Pi Zero 3

This repository contains a C++ program designed to control a CPU cooling fan on an **Orange Pi Zero 3** based on its temperature. It uses the `libgpiod` library to interact with the GPIO pins.

## Features

* Reads CPU temperature from `/sys/class/thermal/thermal_zone2/temp`.

* Turns the fan ON when the temperature reaches `56.0°C`.

* Turns the fan OFF when the temperature drops to `55.5°C`.

* Polls the temperature every `2 seconds`.

* Designed for continuous, low-resource background operation.

* Minimal logging (only errors and warnings are printed).

## Requirements

* An **Orange Pi Zero 3** board.

* A cooling fan connected to GPIO pin 78 (as defined by `LINE_NUMBER`).

* Linux operating system (e.g., Armbian, Debian).

* `libgpiod` development libraries installed.

## Installation

### 1. Install `libgpiod` Development Libraries

On your Orange Pi Zero 3, open a terminal and run:

```
sudo apt update
sudo apt install libgpiod-dev

```

### 2. Compile the Program

Navigate to the directory containing `cpu_fan_control.cpp` and compile it using `g++`:

```
g++ -o cpu_fan_control cpu_fan_control.cpp -lgpiod

```

### 3. Make the Executable Runnable

Give execute permissions to the compiled program:

```
chmod +x cpu_fan_control

```

## Usage

### Running Manually (for testing)

You can run the program directly from your terminal. Since it interacts with hardware, it typically requires root privileges:

```
sudo ./cpu_fan_control

```

Press `Ctrl+C` to stop the program.

### Running as a Systemd Service (Recommended for 24/7 operation)

To ensure the fan control program starts automatically on boot and runs continuously in the background, set it up as a `systemd` service.

1. **Create the service file:**

   ```
   sudo nano /etc/systemd/system/cpu-fan-control.service
   
   ```

   Paste the following content into the file:

   ```
   [Unit]
   Description=CPU Fan Control Service
   After=network-online.target
   
   [Service]
   ExecStart=/usr/bin/sudo /home/wan/cpu_fan_control
   Restart=on-failure
   User=root
   Group=root
   StandardOutput=journal
   StandardError=journal
   
   [Install]
   WantedBy=multi-user.target
   
   ```

   **Note:** Ensure the `ExecStart` path (`/home/wan/cpu_fan_control`) matches the actual location of your compiled executable.

2. **Reload `systemd` daemon:**

   ```
   sudo systemctl daemon-reload
   
   ```

3. **Enable the service (to start on boot):**

   ```
   sudo systemctl enable cpu-fan-control.service
   
   ```

4. **Start the service now:**

   ```
   sudo systemctl start cpu-fan-control.service
   
   ```

### Checking Status and Logs

* **Check service status:**

  ```
  sudo systemctl status cpu-fan-control.service
  
  ```

* **View live logs:**

  ```
  sudo journalctl -u cpu-fan-control.service -f
  
  ```

This README provides a clear overview and instructions for anyone looking to use your CPU fan control program. Let me know if you'd like any other sections or details added!
