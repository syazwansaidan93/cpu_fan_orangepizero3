#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <gpiod.h>
#include <cstring> // For strerror
#include <iomanip> // For std::setprecision and std::fixed

const char* CHIP_NAME = "gpiochip1";
const unsigned int LINE_NUMBER = 78;
const char* CONSUMER = "cpu_temp_fan_control";
const double FAN_ON_TEMP = 56.0;
const double FAN_OFF_TEMP = 55.5;
const std::chrono::seconds POLLING_INTERVAL = std::chrono::seconds(2);
const char* TEMP_PATH = "/sys/class/thermal/thermal_zone2/temp";

double readCPUTemp() {
    std::ifstream tempFile(TEMP_PATH);
    if (!tempFile.is_open()) {
        throw std::runtime_error(
            std::string("Could not open CPU temperature file: ") + TEMP_PATH +
            ". Make sure the path is correct and you have read permissions."
        );
    }

    std::string tempStr;
    std::getline(tempFile, tempStr);

    try {
        int tempRaw = std::stoi(tempStr);
        return static_cast<double>(tempRaw) / 1000.0;
    } catch (const std::invalid_argument& e) {
        throw std::runtime_error(
            std::string("Could not parse temperature value '") + tempStr + "': " + e.what()
        );
    } catch (const std::out_of_range& e) {
        throw std::runtime_error(
            std::string("Temperature value '") + tempStr + "' out of range: " + e.what()
        );
    }
}

int main() {
    gpiod_chip* chip = nullptr;
    gpiod_line* line = nullptr;

    try {
        chip = gpiod_chip_open_by_name(CHIP_NAME);
        if (!chip) {
            throw std::runtime_error(
                std::string("Could not open GPIO chip '") + CHIP_NAME +
                "'. Make sure the chip name is correct and you have appropriate permissions (try running with 'sudo')."
            );
        }
        // std::cerr << "INFO: Opened GPIO chip '" << CHIP_NAME << "'." << std::endl; // Disabled log

        line = gpiod_chip_get_line(chip, LINE_NUMBER);
        if (!line) {
            throw std::runtime_error(
                std::string("Could not get GPIO line '") + std::to_string(LINE_NUMBER) +
                "' on chip '" + CHIP_NAME + "'. Make sure the line number is correct."
            );
        }
        // std::cerr << "INFO: Got GPIO line '" << LINE_NUMBER << "'." << std::endl; // Disabled log

        int ret = gpiod_line_request_output(line, CONSUMER, 0);
        if (ret < 0) {
            throw std::runtime_error(
                std::string("Could not request GPIO line '") + std::to_string(LINE_NUMBER) +
                "' on chip '" + CHIP_NAME + "' as output: " + strerror(errno) +
                ". Make sure the GPIO line is not in use and you have appropriate permissions (try running with 'sudo')."
            );
        }
        // std::cerr << "INFO: Requested GPIO line '" << LINE_NUMBER << "' as output with initial value 0." << std::endl; // Disabled log


        bool fanIsOn = false;
        // std::cerr << "INFO: Starting CPU fan control. Fan ON at " << FAN_ON_TEMP // Disabled log
        //           << "°C, OFF at " << FAN_OFF_TEMP << "°C. Polling every " // Disabled log
        //           << POLLING_INTERVAL.count() << " seconds." << std::endl; // Disabled log

        while (true) {
            double temp;
            try {
                temp = readCPUTemp();
            } catch (const std::runtime_error& e) {
                std::cerr << "ERROR: " << e.what() << std::endl;
                std::this_thread::sleep_for(POLLING_INTERVAL);
                continue;
            }

            if (temp >= FAN_ON_TEMP && !fanIsOn) {
                ret = gpiod_line_set_value(line, 1);
                if (ret < 0) {
                    std::cerr << "ERROR: Could not set GPIO line to ON: " << strerror(errno) << std::endl;
                } else {
                    fanIsOn = true;
                    // std::cerr << "INFO: Temperature: " << std::fixed << std::setprecision(2) << temp // Disabled log
                    //           << "°C (>= " << std::fixed << std::setprecision(1) << FAN_ON_TEMP // Disabled log
                    //           << "°C). Turning fan ON." << std::endl; // Disabled log
                }
            } else if (temp <= FAN_OFF_TEMP && fanIsOn) {
                ret = gpiod_line_set_value(line, 0);
                if (ret < 0) {
                    std::cerr << "ERROR: Could not set GPIO line to OFF: " << strerror(errno) << std::endl;
                } else {
                    fanIsOn = false;
                    // std::cerr << "INFO: Temperature: " << std::fixed << std::setprecision(2) << temp // Disabled log
                    //           << "°C (<= " << std::fixed << std::setprecision(1) << FAN_OFF_TEMP // Disabled log
                    //           << "°C). Turning fan OFF." << std::endl; // Disabled log
                }
            } else {
                // No change in fan state, no info log
            }

            std::this_thread::sleep_for(POLLING_INTERVAL);
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }

    if (line) {
        if (gpiod_line_set_value(line, 0) < 0) {
            std::cerr << "WARNING: Could not set GPIO line to 0 before releasing: " << strerror(errno) << std::endl;
        }
        // std::cerr << "INFO: GPIO line released." << std::endl; // Disabled log
    }
    if (chip) {
        // std::cerr << "INFO: GPIO chip closed." << std::endl; // Disabled log
        gpiod_chip_close(chip);
    }

    return 0;
}
