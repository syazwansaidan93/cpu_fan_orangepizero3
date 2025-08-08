#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <gpiod.h>

const char* CHIP_NAME = "gpiochip1";
const unsigned int LINE_NUMBER = 78;
const char* CONSUMER = "cpu_temp_fan_control";
const double FAN_ON_TEMP = 56.0;
const double FAN_OFF_TEMP = 55.5;
const unsigned int POLLING_INTERVAL_SECONDS = 3;
const char* TEMP_PATH = "/sys/class/thermal/thermal_zone2/temp";

double read_cpu_temp() {
    FILE* temp_file = fopen(TEMP_PATH, "r");
    if (!temp_file) {
        fprintf(stderr, "ERROR: Could not open CPU temperature file: %s. %s\n", TEMP_PATH, strerror(errno));
        return -1.0;
    }

    char temp_str[16];
    if (fgets(temp_str, sizeof(temp_str), temp_file) == NULL) {
        fprintf(stderr, "ERROR: Could not read temperature from file: %s. %s\n", TEMP_PATH, strerror(errno));
        fclose(temp_file);
        return -1.0;
    }
    fclose(temp_file);

    int temp_raw = atoi(temp_str);
    return (double)temp_raw / 1000.0;
}

int main() {
    struct gpiod_chip* chip = NULL;
    struct gpiod_line* line = NULL;

    int ret;
    int fan_is_on = 0;
    double temp;

    chip = gpiod_chip_open_by_name(CHIP_NAME);
    if (!chip) {
        fprintf(stderr, "FATAL ERROR: Could not open GPIO chip '%s'. %s\n", CHIP_NAME, strerror(errno));
        goto cleanup;
    }

    line = gpiod_chip_get_line(chip, LINE_NUMBER);
    if (!line) {
        fprintf(stderr, "FATAL ERROR: Could not get GPIO line %u on chip '%s'. %s\n", LINE_NUMBER, CHIP_NAME, strerror(errno));
        goto cleanup;
    }

    ret = gpiod_line_request_output(line, CONSUMER, 0);
    if (ret < 0) {
        fprintf(stderr, "FATAL ERROR: Could not request GPIO line %u as output. %s\n", LINE_NUMBER, strerror(errno));
        goto cleanup;
    }

    while (1) {
        temp = read_cpu_temp();
        if (temp < 0.0) {
            sleep(POLLING_INTERVAL_SECONDS);
            continue;
        }

        if (temp >= FAN_ON_TEMP && !fan_is_on) {
            ret = gpiod_line_set_value(line, 1);
            if (ret < 0) {
                fprintf(stderr, "ERROR: Could not set GPIO line to ON. %s\n", strerror(errno));
            } else {
                fan_is_on = 1;
            }
        } else if (temp <= FAN_OFF_TEMP && fan_is_on) {
            ret = gpiod_line_set_value(line, 0);
            if (ret < 0) {
                fprintf(stderr, "ERROR: Could not set GPIO line to OFF. %s\n", strerror(errno));
            } else {
                fan_is_on = 0;
            }
        }

        sleep(POLLING_INTERVAL_SECONDS);
    }

cleanup:
    if (line) {
        gpiod_line_set_value(line, 0);
        gpiod_line_release(line);
    }
    if (chip) {
        gpiod_chip_close(chip);
    }

    return (ret < 0) ? 1 : 0;
}
