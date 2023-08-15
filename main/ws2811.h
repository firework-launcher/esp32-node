#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt.h"
#include "esp_log.h"

#define WS2811_CHANNEL    RMT_CHANNEL_0
#define WS2811_GPIO_NUM   35
#define WS2811_LED_NUM    16

#define T0H  12
#define T1H  25
#define T0L  37
#define T1L  25

rmt_item32_t led_data[24 * WS2811_LED_NUM] = {0};

static const char* WS2811_TAG = "LED";

void ws2811_init() {
    rmt_config_t config = {
        .channel = WS2811_CHANNEL,
        .gpio_num = WS2811_GPIO_NUM,
        .clk_div = 2,
        .rmt_mode = RMT_MODE_TX,
        .mem_block_num = 1,
        .tx_config.loop_en = false,
        .tx_config.carrier_en = false,
        .tx_config.idle_output_en = true,
        .tx_config.idle_level = 0,
    };
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    ESP_LOGI(WS2811_TAG, "LEDs initialized");
}

void ws2811_set_leds() {
    rmt_write_items(WS2811_CHANNEL, led_data, 24 * WS2811_LED_NUM, true);
}


void ws2811_set_rgb(uint8_t led_index, uint8_t green, uint8_t red, uint8_t blue) {
    if (led_index >= WS2811_LED_NUM) return;

    uint32_t bits_to_send = (green << 16) | (red << 8) | blue;
    int32_t mask = 1 << (23);
    for (int bit = 0; bit < 24; bit++) {
        led_data[led_index * 24 + bit].level0 = 1;
        led_data[led_index * 24 + bit].level1 = 0;

        if ((bits_to_send & mask) == 0) {
            led_data[led_index * 24 + bit].duration0 = T0H;
            led_data[led_index * 24 + bit].duration1 = T0L;
        } else {
            led_data[led_index * 24 + bit].duration0 = T1H;
            led_data[led_index * 24 + bit].duration1 = T1L;
        }
        mask >>= 1;
    }
}
