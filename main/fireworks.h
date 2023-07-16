#include "esp_log.h"
#include "driver/gpio.h"

static char *TAG_FIREWORKS = "FIREWORKS";

void init_gpio(void) {
    gpio_set_direction(27, GPIO_MODE_OUTPUT);
    gpio_set_direction(26, GPIO_MODE_OUTPUT);
    gpio_set_direction(25, GPIO_MODE_OUTPUT);
}

void launch_firework(gpio_num_t firework) {
    gpio_set_level(firework, 1);
    ESP_LOGI(TAG_FIREWORKS, "Pin %d set to 1", firework);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(firework, 0);
    ESP_LOGI(TAG_FIREWORKS, "Pin %d set to 0", firework);
}
