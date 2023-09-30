int ip_digits[3] = {11, 11, 11};

#include "pthread.h"

pthread_t tcprxthread;
pthread_t tcptxthread;
char* wifi_ssid = "Node";
char* wifi_password = "Password";

#include "esp_err.h"
#include "tcp_io.h"
#include "wifi.h"
#include "ota.h"
#include "discovery.h"

static char *TAG_LAUNCHER = "LAUNCHER";

void init_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void get_wifi_details(void) {
    nvs_handle_t wifinvs_handle;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &wifinvs_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
    }
    size_t required_size;
    nvs_get_str(wifinvs_handle, "ssid", NULL, &required_size);
    wifi_ssid = malloc(required_size);
    nvs_get_str(wifinvs_handle, "ssid", wifi_ssid, &required_size);

    nvs_get_str(wifinvs_handle, "passwd", NULL, &required_size);
    wifi_password = malloc(required_size);
    nvs_get_str(wifinvs_handle, "passwd", wifi_password, &required_size);
    if (wifi_password == NULL || wifi_ssid == NULL) {
        entering_ota = true;
    }
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
    cJSON* read_data;

    while (1) {
        if (entering_ota == true) {
            break;
        }

        root = cJSON_CreateObject();

        cJSON_AddNumberToObject(root, "signalStrength", get_signal_strength());

        read_data = read_bytes(0x20);
        cJSON_AddItemToObject(root, "inputData", read_data);

        data = cJSON_PrintUnformatted(root);
        sprintf(msg_buffer_input, "%s\r\n", data);
        free(data);
        cJSON_Delete(root);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG_TCP_TX, "TX Main ended.");
}

void app_main(void)
{
    init_gpio();
    init_nvs();
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 16384);
    get_wifi_details();
    if (entering_ota == true) {
        ota(attr);
    }
    start_wifi_display(attr);
    bool wifi_ret = init_wifi();
    while (wifi_ret != true) {
        ESP_LOGE(TAG_WIFI, "Failed to connect to WiFi");
        entering_ota = true;
        ota(attr);
    }
    start_tcptx_thread(attr);
    start_tcprx_thread(attr);
    start_discovery_thread(attr);
    waiting();
    tx_main();
    ota(attr);
}
