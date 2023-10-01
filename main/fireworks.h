#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "ws2811.h"
#include "io_expander.h"

static char *TAG_FIREWORKS = "FIREWORKS";

int gpio_firework_mappings[16] = {5, 6, 7, 15, 16, 17, 18, 19, 20, 10, 11, 12, 13, 14, 21, 2};

static void ledc_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .freq_hz          = 1000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
}

void ledc_init_channel(int gpio, int channel) {
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = channel,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = gpio,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void init_gpio(void) {
    init_i2c();

    init_output(0x21);
    init_output(0x22);
    init_input(0x20);

    gpio_set_direction(4, GPIO_MODE_OUTPUT); // Arm/Disarm

    gpio_set_direction(39, GPIO_MODE_OUTPUT); // Digit display: A
    gpio_set_direction(42, GPIO_MODE_OUTPUT); // Digit display: B
    gpio_set_direction(41, GPIO_MODE_OUTPUT); // Digit display: C
    gpio_set_direction(40, GPIO_MODE_OUTPUT); // Digit display: D
    gpio_set_direction(47, GPIO_MODE_OUTPUT); // Digit display: Decimal
    gpio_set_level(39, 1);
    gpio_set_level(42, 1);
    gpio_set_level(41, 1);
    gpio_set_level(40, 1);
    gpio_set_level(47, 0);

    // Fireworks
    gpio_set_direction(5, GPIO_MODE_OUTPUT); // Pin 1
    gpio_set_direction(6, GPIO_MODE_OUTPUT); // Pin 2
    gpio_set_direction(7, GPIO_MODE_OUTPUT); // Pin 3
    gpio_set_direction(15, GPIO_MODE_OUTPUT); // Pin 4
    gpio_set_direction(16, GPIO_MODE_OUTPUT); // Pin 5
    gpio_set_direction(17, GPIO_MODE_OUTPUT); // Pin 6
    gpio_set_direction(18, GPIO_MODE_OUTPUT); // Pin 7
    gpio_set_direction(19, GPIO_MODE_OUTPUT); // Pin 8
    gpio_set_direction(20, GPIO_MODE_OUTPUT); // Pin 9
    gpio_set_direction(10, GPIO_MODE_OUTPUT); // Pin 10
    gpio_set_direction(11, GPIO_MODE_OUTPUT); // Pin 11
    gpio_set_direction(12, GPIO_MODE_OUTPUT); // Pin 12
    gpio_set_direction(13, GPIO_MODE_OUTPUT); // Pin 13
    gpio_set_direction(14, GPIO_MODE_OUTPUT); // Pin 14
    gpio_set_direction(21, GPIO_MODE_OUTPUT); // Pin 15
    gpio_set_direction(2, GPIO_MODE_OUTPUT); // Pin 16

    gpio_set_pull_mode(19, GPIO_PULLDOWN_ENABLE);
    gpio_set_pull_mode(20, GPIO_PULLDOWN_ENABLE);

    gpio_set_level(5, 0);
    gpio_set_level(6, 0);
    gpio_set_level(7, 0);
    gpio_set_level(15, 0);
    gpio_set_level(16, 0);
    gpio_set_level(17, 0);
    gpio_set_level(18, 0);
    gpio_set_level(19, 0);
    gpio_set_level(20, 0);
    gpio_set_level(10, 0);
    gpio_set_level(11, 0);
    gpio_set_level(12, 0);
    gpio_set_level(13, 0);
    gpio_set_level(14, 0);
    gpio_set_level(21, 0);
    gpio_set_level(2, 0);
    


    ws2811_init();
    ledc_init();
}

void launch_firework(int gpio, int pwm) {
    gpio -= 1;
    if (armed == true) {
        ws2811_set_rgb(gpio, (int)((float)pwm/7500*255), 0, 0);
        ws2811_set_leds();
        ledc_init_channel(gpio_firework_mappings[gpio], LEDC_CHANNEL_0);
        ESP_LOGI(TAG_FIREWORKS, "Trigger firework called for %d, pwm %d", gpio+1, pwm);
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ws2811_set_rgb(gpio, 0, 0, 0);
        ws2811_set_leds();
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        gpio_reset_pin(gpio_firework_mappings[gpio]);
    } else {
        ESP_LOGW(TAG_FIREWORKS, "Launch firework called while the launcher is unarmed");
    }
}

void run_step(cJSON *fireworks, cJSON *pwm_json) {
    if (armed == true) {
        int gpio;
        int pwm;

        for (int i = 0; i < cJSON_GetArraySize(fireworks); i++) {
            gpio = cJSON_GetArrayItem(fireworks, i)->valueint-1;
            pwm = cJSON_GetArrayItem(pwm_json, i)->valueint;
            ws2811_set_rgb(gpio, (int)((float)pwm/7500*255), 0, 0);
            ws2811_set_leds();
            ledc_init_channel(gpio_firework_mappings[gpio], i);
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, i, pwm));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, i));
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        ws2811_set_leds();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        for (int i = 0; i < cJSON_GetArraySize(fireworks); i++) {
            gpio = cJSON_GetArrayItem(fireworks, i)->valueint-1;
            ws2811_set_rgb(gpio, 0, 0, 0);
            ws2811_set_leds();
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, i, 0));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, i));
            ledc_stop(LEDC_LOW_SPEED_MODE, i, 0);
            gpio_reset_pin(gpio_firework_mappings[gpio]);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        ws2811_set_leds();
    }
}
