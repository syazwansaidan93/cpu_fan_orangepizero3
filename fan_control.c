#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <jansson.h>

const char* CONFIG_FILE = "/home/wan/fan_control/config.json";
const char* GPIO_EXPORT_PATH = "/sys/class/gpio/export";
const char* GPIO_UNEXPORT_PATH = "/sys/class/gpio/unexport";
const char* GPIO_BASE_PATH = "/sys/class/gpio/";

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
    int ret = -1;

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

    ret = 0;
    
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

int set_gpio_value(const char* path, int value) {
    FILE* fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: Could not open GPIO value file at %s. %s\n", path, strerror(errno));
        return -1;
    }
    fprintf(fp, "%d", value);
    fclose(fp);
    return 0;
}

int main() {
    fan_config config = {0};
    int ret = 0;
    int fan_is_on = 0;
    double temp;
    char gpio_dir_path[256];
    char gpio_value_path[256];
    char gpio_pin_str[16];

    if (load_config(CONFIG_FILE, &config) < 0) {
        return 1;
    }

    // Convert line number to string for path construction
    snprintf(gpio_pin_str, sizeof(gpio_pin_str), "%u", config.line_number);

    // Export the GPIO pin
    FILE* export_fp = fopen(GPIO_EXPORT_PATH, "w");
    if (!export_fp) {
        fprintf(stderr, "FATAL ERROR: Could not open GPIO export file. %s\n", strerror(errno));
        ret = -1;
        goto cleanup;
    }
    fprintf(export_fp, "%s", gpio_pin_str);
    fclose(export_fp);

    // Give the system a moment to create the directory
    usleep(100000); 

    // Construct the path for the GPIO direction and value files
    snprintf(gpio_dir_path, sizeof(gpio_dir_path), "%sgpio%s/direction", GPIO_BASE_PATH, gpio_pin_str);
    snprintf(gpio_value_path, sizeof(gpio_value_path), "%sgpio%s/value", GPIO_BASE_PATH, gpio_pin_str);
    
    // Set the GPIO direction to output
    FILE* dir_fp = fopen(gpio_dir_path, "w");
    if (!dir_fp) {
        fprintf(stderr, "FATAL ERROR: Could not open GPIO direction file. %s\n", strerror(errno));
        ret = -1;
        goto cleanup;
    }
    fprintf(dir_fp, "out");
    fclose(dir_fp);

    // Set initial value to 0 to ensure the fan is off
    if (set_gpio_value(gpio_value_path, 0) < 0) {
        ret = -1;
        goto cleanup;
    }

    while (1) {
        temp = read_cpu_temp(config.temp_path);
        if (temp < 0.0) {
            sleep(config.polling_interval_seconds);
            continue;
        }

        if (temp >= config.fan_on_temp && !fan_is_on) {
            if (set_gpio_value(gpio_value_path, 1) < 0) {
                fprintf(stderr, "ERROR: Could not set GPIO line to ON. %s\n", strerror(errno));
            } else {
                fan_is_on = 1;
                fprintf(stderr, "DEBUG: Temperature %.2f C is above threshold, turning fan ON.\n", temp);
            }
        } else if (temp <= config.fan_off_temp && fan_is_on) {
            if (set_gpio_value(gpio_value_path, 0) < 0) {
                fprintf(stderr, "ERROR: Could not set GPIO line to OFF. %s\n", strerror(errno));
            } else {
                fan_is_on = 0;
                fprintf(stderr, "DEBUG: Temperature %.2f C is below threshold, turning fan OFF.\n", temp);
            }
        }
        sleep(config.polling_interval_seconds);
    }

cleanup:
    // Ensure the fan is off and the pin is unexported on exit
    set_gpio_value(gpio_value_path, 0);

    FILE* unexport_fp = fopen(GPIO_UNEXPORT_PATH, "w");
    if (unexport_fp) {
        fprintf(unexport_fp, "%s", gpio_pin_str);
        fclose(unexport_fp);
    }
    
    free_config(&config);

    return (ret < 0) ? 1 : 0;
}
