#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <gpiod.h>
#include <jansson.h>

const char* CONFIG_FILE = "/home/wan/fan_control/config.json";

typedef struct {
    char* chip_name;
    unsigned int line_number;
    char* consumer;
    double fan_on_temp;
    double fan_off_temp;
    unsigned int polling_interval_seconds;
    char* temp_path;
} fan_config;

void free_config(fan_config* config) {
    if (config->chip_name) free(config->chip_name);
    if (config->consumer) free(config->consumer);
    if (config->temp_path) free(config->temp_path);
}

int load_config(const char* filename, fan_config* config) {
    json_t* root;
    json_error_t error;
    int ret = -1; // Default to failure

    root = json_load_file(filename, 0, &error);
    if (!root) {
        fprintf(stderr, "FATAL ERROR: Could not open or parse JSON config file: %s\n", error.text);
        return -1;
    }

    json_t *chip_name_json = json_object_get(root, "chip_name");
    if (!chip_name_json || !json_is_string(chip_name_json)) {
        fprintf(stderr, "FATAL ERROR: 'chip_name' not found or is not a string in config.json.\n");
        goto cleanup;
    }
    config->chip_name = strdup(json_string_value(chip_name_json));
    if (!config->chip_name) {
        fprintf(stderr, "FATAL ERROR: Memory allocation failed for chip_name.\n");
        goto cleanup;
    }

    json_t *line_number_json = json_object_get(root, "line_number");
    if (!line_number_json || !json_is_integer(line_number_json)) {
        fprintf(stderr, "FATAL ERROR: 'line_number' not found or is not an integer in config.json.\n");
        goto cleanup;
    }
    config->line_number = (unsigned int)json_integer_value(line_number_json);

    json_t *consumer_json = json_object_get(root, "consumer");
    if (!consumer_json || !json_is_string(consumer_json)) {
        fprintf(stderr, "FATAL ERROR: 'consumer' not found or is not a string in config.json.\n");
        goto cleanup;
    }
    config->consumer = strdup(json_string_value(consumer_json));
    if (!config->consumer) {
        fprintf(stderr, "FATAL ERROR: Memory allocation failed for consumer.\n");
        goto cleanup;
    }

    json_t *fan_on_temp_json = json_object_get(root, "fan_on_temp");
    if (!fan_on_temp_json || !json_is_number(fan_on_temp_json)) {
        fprintf(stderr, "FATAL ERROR: 'fan_on_temp' not found or is not a number in config.json.\n");
        goto cleanup;
    }
    config->fan_on_temp = json_real_value(fan_on_temp_json);

    json_t *fan_off_temp_json = json_object_get(root, "fan_off_temp");
    if (!fan_off_temp_json || !json_is_number(fan_off_temp_json)) {
        fprintf(stderr, "FATAL ERROR: 'fan_off_temp' not found or is not a number in config.json.\n");
        goto cleanup;
    }
    config->fan_off_temp = json_real_value(fan_off_temp_json);

    json_t *polling_interval_json = json_object_get(root, "polling_interval_seconds");
    if (!polling_interval_json || !json_is_integer(polling_interval_json)) {
        fprintf(stderr, "FATAL ERROR: 'polling_interval_seconds' not found or is not an integer in config.json.\n");
        goto cleanup;
    }
    config->polling_interval_seconds = (unsigned int)json_integer_value(polling_interval_json);

    json_t *temp_path_json = json_object_get(root, "temp_path");
    if (!temp_path_json || !json_is_string(temp_path_json)) {
        fprintf(stderr, "FATAL ERROR: 'temp_path' not found or is not a string in config.json.\n");
        goto cleanup;
    }
    config->temp_path = strdup(json_string_value(temp_path_json));
    if (!config->temp_path) {
        fprintf(stderr, "FATAL ERROR: Memory allocation failed for temp_path.\n");
        goto cleanup;
    }

    ret = 0; // Success
    
cleanup:
    json_decref(root);
    return ret;
}

double read_cpu_temp(const char* temp_path) {
    FILE* temp_file = fopen(temp_path, "r");
    if (!temp_file) {
        fprintf(stderr, "ERROR: Could not open CPU temperature file: %s. %s\n", temp_path, strerror(errno));
        return -1.0;
    }

    char temp_str[16];
    if (fgets(temp_str, sizeof(temp_str), temp_file) == NULL) {
        fprintf(stderr, "ERROR: Could not read temperature from file: %s. %s\n", temp_path, strerror(errno));
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
    fan_config config = {0}; // Initialize to all zeros
    int ret = 0;
    int fan_is_on = 0;
    double temp;

    if (load_config(CONFIG_FILE, &config) < 0) {
        return 1;
    }

    chip = gpiod_chip_open_by_name(config.chip_name);
    if (!chip) {
        fprintf(stderr, "FATAL ERROR: Could not open GPIO chip '%s'. %s\n", config.chip_name, strerror(errno));
        ret = -1;
        goto cleanup;
    }

    line = gpiod_chip_get_line(chip, config.line_number);
    if (!line) {
        fprintf(stderr, "FATAL ERROR: Could not get GPIO line %u on chip '%s'. %s\n", config.line_number, config.chip_name, strerror(errno));
        ret = -1;
        goto cleanup;
    }

    ret = gpiod_line_request_output(line, config.consumer, 0);
    if (ret < 0) {
        fprintf(stderr, "FATAL ERROR: Could not request GPIO line %u as output. %s\n", config.line_number, strerror(errno));
        goto cleanup;
    }
    
    gpiod_line_set_value(line, 0);

    while (1) {
        temp = read_cpu_temp(config.temp_path);
        if (temp < 0.0) {
            sleep(config.polling_interval_seconds);
            continue;
        }

        if (temp >= config.fan_on_temp && !fan_is_on) {
            ret = gpiod_line_set_value(line, 1);
            if (ret < 0) {
                fprintf(stderr, "ERROR: Could not set GPIO line to ON. %s\n", strerror(errno));
            } else {
                fan_is_on = 1;
            }
        } else if (temp <= config.fan_off_temp && fan_is_on) {
            ret = gpiod_line_set_value(line, 0);
            if (ret < 0) {
                fprintf(stderr, "ERROR: Could not set GPIO line to OFF. %s\n", strerror(errno));
            } else {
                fan_is_on = 0;
            }
        }

        sleep(config.polling_interval_seconds);
    }

cleanup:
    if (line) {
        gpiod_line_set_value(line, 0);
        gpiod_line_release(line);
    }
    if (chip) {
        gpiod_chip_close(chip);
    }
    free_config(&config);

    return (ret < 0) ? 1 : 0;
}
