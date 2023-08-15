int ip_digits[3] = {11, 11, 11};

#include "esp_err.h"
#include "tcp_io.h"
#include "wifi.h"

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

void tx_main(void) {
    char *data;
    cJSON *root;
    int inputData[2];
    cJSON* inputDataArrayJSON;
    cJSON* inputDataEntryJSON;

    while (1) {
        root = cJSON_CreateObject();

        cJSON_AddNumberToObject(root, "signalStrength", get_signal_strength());

        
        cJSON_AddItemToObject(root, "inputData", read_bytes(0x20));

        data = cJSON_PrintUnformatted(root);
        sprintf(msg_buffer_input, "%s\r\n", data);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    init_nvs();
    init_gpio();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 16384);
    start_wifi_display(attr);
    bool wifi_ret = init_wifi();
    while (wifi_ret != true) {
        ESP_LOGE(TAG_WIFI, "Failed to connect to WiFi, trying again..");
        wifi_ret = init_wifi();
    }
    start_tcptx_thread(attr);
    start_tcprx_thread(attr);
    waiting();
    tx_main();
}
