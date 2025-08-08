# CPU Fan Control for Orange Pi Zero 3

This project provides a simple C program to automatically control a cooling fan on an Orange Pi Zero 3 based on the CPU temperature. It reads the temperature from a specified thermal zone and toggles a GPIO pin to turn the fan on or off.

### Dependencies

To compile and run this program, you need:

* **GCC**: The C compiler.

* **`libgpiod-dev`**: The development library for `libgpiod`, which provides a modern interface for GPIO access on Linux systems.

You can install these dependencies on Armbian with the following commands:

```
sudo apt-get update
sudo apt-get install gcc libgpiod-dev

```

### Compilation

Navigate to the directory where you saved the C source file (`fan_control.c`) and compile it using GCC. The `-lgpiod` flag is crucial for linking the `libgpiod` library.

```
gcc fan_control.c -o fan_control -lgpiod

```

### Configuration

You can easily customize the program's behavior by editing the constant values in the source code before compiling.

* `CHIP_NAME`: The name of the GPIO chip.

* `LINE_NUMBER`: The specific GPIO line number connected to your fan.

* `FAN_ON_TEMP`: The temperature in degrees Celsius at which the fan will turn on.

* `FAN_OFF_TEMP`: The temperature in degrees Celsius at which the fan will turn off.

* `POLLING_INTERVAL_SECONDS`: The interval in seconds between temperature checks.

* `TEMP_PATH`: The path to the thermal zone file that reports the CPU temperature.

### Running as a systemd Service

For persistent and automatic operation, it is recommended to run this program as a systemd service.

1. **Create the service file**: Create a new file at `/etc/systemd/system/fan_control.service`.

   ```
   sudo nano /etc/systemd/system/fan_control.service
   
   ```

2. **Add the following content**: Ensure the `ExecStart` path points to the correct location of your compiled program.

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

3. **Enable and start the service**:

   ```
   sudo systemctl daemon-reload
   sudo systemctl enable fan_control.service
   sudo systemctl start fan_control.service
   
   
