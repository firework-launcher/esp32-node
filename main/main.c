#include "wifi.h"
#include "tcp_io.h"

static char *TAG_LAUNCHER = "LAUNCHER";

void init_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

bool init_wifi(void) {
    return wifi_init_sta();
}

void launcher_main(void) {
    while (1) {
        // sprintf(msg_buffer_input, "%d\r\n", get_signal_strength());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    init_nvs();
    bool wifi_ret = init_wifi();
    if (wifi_ret == true) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 16384);
        start_tcptx_thread(attr);
        start_tcprx_thread(attr);
        init_gpio();
        launcher_main();
    }
}
