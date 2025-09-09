# CPU Fan Control for Orange Pi Zero 3

This project provides a simple C program to automatically control a cooling fan on an Orange Pi Zero 3 based on the CPU temperature. It reads the temperature from a specified thermal zone and toggles a GPIO pin to turn the fan on or off.

### Dependencies

To compile and run this program, you need:

* **GCC**: The C compiler.
* **`libjansson`**: The Jansson library for parsing JSON configuration files.

You can install these dependencies on Armbian with the following commands:

```
sudo apt-get update
sudo apt-get install gcc libjansson
```

### Configuration

The program reads its configuration from a `config.json` file. The path to this file is currently hardcoded in the C source file as `/home/wan/fan_control/config.json`. You must place the `config.json` file in this specific location for the program to find it.

Here is an example of the `config.json` file:

```json
{
  "chip_name": "gpiochip1",
  "line_number": 78,
  "consumer": "cpu_temp_fan_control",
  "fan_on_temp": 56.0,
  "fan_off_temp": 55.5,
  "polling_interval_seconds": 3,
  "temp_path": "/sys/class/thermal/thermal_zone2/temp"
}
```

### Compilation

Navigate to the directory where you saved the C source file (`fan_control.c`) and compile it using GCC. The `-ljansson` flags are crucial for linking the necessary libraries.

```
gcc fan_control.c -o fan_control -ljansson
```

### Usage

You can run the compiled program directly from the command line to test it. Since it requires GPIO access, you may need to use `sudo`.

```bash
sudo /home/wan/fan_control/fan_control
```

The program will print its current status, including the CPU temperature and the fan's state, to the console. This is useful for debugging and verifying that the temperature thresholds are working as expected.

### Running as a systemd Service

For persistent and automatic operation, it is recommended to run this program as a systemd service.

1.  **Create the service file**: Create a new file at `/etc/systemd/system/fan_control.service`.

    ```
    sudo nano /etc/systemd/system/fan_control.service
    ```

2.  **Add the following content**: Ensure the `ExecStart` path points to the correct location of your compiled program.

    ```
    [Unit]
    Description=CPU Fan Control Service
    After=network.target

    [Service]
    Type=simple
    User=root
    Group=root
    ExecStart=/home/wan/fan_control/fan_control
    Restart=always
    RestartSec=5

    [Install]
    WantedBy=multi-user.target
    ```

3.  **Enable and start the service**:

    ```
    sudo systemctl daemon-reload
    sudo systemctl enable fan_control.service
    sudo systemctl start fan_control.service
    
