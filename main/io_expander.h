#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "cJSON.h"

#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0

static const char* TAG_I2C = "I2C";

int mappings[16] = {4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11};
int dispboard_pca_pins[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int byte_mappings[16] = {4, 7, 6, 5, 3, 15, 2, 1, 0, 14, 10, 13, 12, 11, 9, 8};

bool tx_connected = false;
bool rx_connected = false;
bool armed = false;
bool leds_stopped = false;

static esp_err_t init_i2c(void) {
    int i2c_master_port = 0;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 8,
        .scl_io_num = 9,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

static esp_err_t write_bytes(uint8_t address, uint8_t reg, uint8_t data1, uint8_t data2) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data2 | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t get_read_ready(uint8_t address) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void set_led_colors(uint16_t value1, uint16_t value2) {
    for (int i = 0; i < 8; i++) {
        uint8_t bit1 = (value1 >> i) & 0x01;
        uint8_t bit2 = (value2 >> i) & 0x01;
        uint8_t led_index = i;

        if (bit1 == 0) {
            ws2811_set_rgb(mappings[led_index], 20, 0, 0);
        } else {
            ws2811_set_rgb(mappings[led_index], 0, 20, 0);
        }

        if (bit2 == 0) {
            ws2811_set_rgb(mappings[led_index+8], 20, 0, 0);
        } else {
            ws2811_set_rgb(mappings[led_index+8], 0, 20, 0);
        }
    }
    ws2811_set_leds();
}

void set_dispboard_pca_pins(int pin, int state) {
    dispboard_pca_pins[pin] = state;
}

void write_dispboard_pca(int pins[16]) {
    uint8_t data1 = 0;
    uint8_t data2 = 0;
    for (int i = 0; i < 8; i++) {
        data1 = (data1 << 1) | pins[i];
    }
    for (int i = 8; i < 16; i++) {
        data2 = (data2 << 1) | pins[i];
    }
    
    write_bytes(0x22, 2, data1, data2);
}

void set_dispboard_pca(uint16_t digit2, uint16_t digit3, uint16_t digit1, uint16_t armPin) {
    int dispboard_pca_pins[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int segdisplay_numbers[3][12][16] = {
        {
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0},
            {0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0},
            {0,0,0,0,1,0,1,1,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0},
            {0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0}
        },
        {
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
            {0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
            {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
            {0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1},
            {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1}
        },
        {
            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
            {0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0},
            {0,0,0,0,0,0,0,0,0,0,1,0,1,1,0,0},
            {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0},
            {0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0}
        }
    };
    int arm[16] = {0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0};
    for (int i = 0; i < 16; i++) {
        dispboard_pca_pins[i] += segdisplay_numbers[0][digit3][i];
    }

    for (int i = 0; i < 16; i++) {
        dispboard_pca_pins[i] += segdisplay_numbers[1][digit1][i];
    }

    for (int i = 0; i < 16; i++) {
        dispboard_pca_pins[i] += segdisplay_numbers[2][digit2][i];
    }
    if (armPin == 1) {
        for (int i = 0; i < 16; i++) {
            dispboard_pca_pins[i] += arm[i];
        }
    }
    write_dispboard_pca(dispboard_pca_pins);
}

cJSON* read_bytes(uint8_t address) {
    uint8_t read[2];
    get_read_ready(address);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &read[0], I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &read[1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    cJSON* json_array = cJSON_CreateArray();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_I2C, "Failed to read from PCA9535, trying again..");
    } else if (ret == ESP_OK) {
        if (armed == true) {
            leds_stopped = true;
        } else {
            set_led_colors(read[0], read[1]);
        }
        cJSON* json_data1 = cJSON_CreateNumber(read[0]);
        cJSON* json_data2 = cJSON_CreateNumber(read[1]);
        cJSON_AddItemToArray(json_array, json_data1);
        cJSON_AddItemToArray(json_array, json_data2);
    }
    return json_array;
}

void init_output(uint8_t address) {
    write_bytes(address, 4, 0x0, 0x0);
    write_bytes(address, 6, 0x0, 0x0);
    write_bytes(address, 2, 0x0, 0x0);
}

void init_input(uint8_t address) {
    write_bytes(address, 4, 0x0, 0x0);
    write_bytes(address, 6, 0xFF, 0xFF);
}

void setPin(uint8_t address, uint8_t pin) {
    int data1 = 0;
    int data2 = 0;
    if (pin > 7) {
        pin -= 8;
        data2 |= (1 << pin);
    } else {
        data1 |= (1 << pin);
    }
    write_bytes(address, 2, data1, data2);
}

void all_off(uint8_t address) {
    write_bytes(address, 2, 0x0, 0x0);
}

void all_on(uint8_t address) {
    write_bytes(address, 2, 0xFF, 0xFF);
}

void waiting(void) {
    uint8_t pin = 0;
    while (rx_connected == false && tx_connected == false) {
        if (entering_ota) {
            break;
        }
        for (int i = 0; i < 16; i++) {
            if (rx_connected == false && tx_connected == false) {
                ws2811_set_rgb(i, 20, 0, 0);
                ws2811_set_leds();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            } else {
                break;
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        for (int i = 0; i < 16; i++) {
            if (rx_connected == false && tx_connected == false) {
                ws2811_set_rgb(i, 0, 20, 0);
                ws2811_set_leds();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            } else {
                break;
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        for (int i = 0; i < 16; i++) {
            if (rx_connected == false && tx_connected == false) {
                ws2811_set_rgb(i, 0, 0, 20);
                ws2811_set_leds();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            } else {
                break;
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void arm(void) {
    armed = true;
    gpio_set_level(4, 1);
    set_dispboard_pca(ip_digits[2], ip_digits[1], ip_digits[0], 1);
    while (leds_stopped == false) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    ws2811_set_all(0, 0, 0);
    ws2811_set_leds();
}

void arm_no_lights(void) {
    armed = true;
    gpio_set_level(4, 1);
}

void disarm(void) {
    armed = false;
    leds_stopped = false;
    gpio_set_level(4, 0);
    set_dispboard_pca(ip_digits[2], ip_digits[1], ip_digits[0], 0);
}

void disarm_no_lights(void) {
    armed = false;
    gpio_set_level(4, 0);
}