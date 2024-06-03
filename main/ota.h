#include <string.h>

#include <freertos/FreeRTOS.h>
#include <esp_http_server.h>
#include <freertos/task.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <sys/param.h>
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include <sys/stat.h>

static httpd_handle_t http_server = NULL;

int node_id;

const char* get_mime_type(const char* filename) {
    if (strstr(filename, ".html")) return "text/html";
    if (strstr(filename, ".css")) return "text/css";
    if (strstr(filename, ".js")) return "application/javascript";
    if (strstr(filename, ".jpg")) return "image/jpeg";
    if (strstr(filename, ".png")) return "image/png";
    return "text/plain";  // default MIME type
}

esp_err_t spiffs_file_handler(httpd_req_t *req) {
    char filepath[ESP_VFS_PATH_MAX + 128];
    struct stat file_stat;

    // Construct the file path
    strcpy(filepath, "/spiffs");
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strcat(filepath, "/index.html"); // Default to index.html if accessing root directory
    } else {
        strcat(filepath, req->uri);
    }

    // Check if the file exists
    if (stat(filepath, &file_stat) == -1) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // Open the file
    FILE *file = fopen(filepath, "r");
    if (!file) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // Fetch the MIME type based on file extension if needed
    const char* mime_type = get_mime_type(filepath);
    httpd_resp_set_type(req, mime_type);

    // Send file content
    char *buffer = malloc(file_stat.st_size);
    fread(buffer, 1, file_stat.st_size, file);
    httpd_resp_send(req, buffer, file_stat.st_size);

    // Clean up
    free(buffer);
    fclose(file);
    return ESP_OK;
}

esp_err_t restart_get_handler(httpd_req_t *req)
{
    esp_restart();
    const char resp[] = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t version_get_handler(httpd_req_t *req)
{
    const char resp[5];
    sprintf(resp, "%d", version);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void set_nvs_keyvalue(char* key, char value[100]) {
    nvs_handle_t wifinvs_handle;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &wifinvs_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
    }
    ESP_LOGI("OTA", "NVS Value length: %d", strlen(value));
    nvs_set_str(wifinvs_handle, key, value);
}

void set_nvs_keyvalue_int(char* key, uint16_t value) {
    nvs_handle_t wifinvs_handle;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &wifinvs_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
    }
    nvs_set_u16(wifinvs_handle, key, value);
}

int random_number(int min_num, int max_num) {
    if (min_num > max_num) {
        int temp = min_num;
        min_num = max_num;
        max_num = temp;
    }
    
    uint32_t range = max_num - min_num + 1;
    return esp_random() % range + min_num;
}

esp_err_t configure_wifipasswd_post_handler(httpd_req_t *req) {
    char content[101]; // Increase buffer size by 1 for null terminator

    memset(content, 0, sizeof(content));

    /* Truncate if content length larger than the buffer minus 1 (to allow for null terminator) */
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    content[ret] = '\0';  // Null terminate the received data
    ESP_LOGI("WIFI CONFIG", "%s", content);
    set_nvs_keyvalue("passwd", content);
    const char resp[] = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    esp_restart();
    return ESP_OK;
}

esp_err_t configure_wifissid_post_handler(httpd_req_t *req) {
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    content[ret] = '\0';  // Null terminate the received data
    set_nvs_keyvalue("ssid", content);
    const char resp[] = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t run_command_post_handler(httpd_req_t *req) {
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    strcpy(recv_buffer, content);
    process_rx_data(recv_buffer);
    const char resp[] = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t setdisplay_post_handler(httpd_req_t *req) {
    char content[16];
    int int_content[16];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    for (int i = 0; i < 16; i++) {
        if (content[i] == 48) {
            int_content[i] = 0;
        } else {
            int_content[i] = 1;
        }
    }
    
    write_dispboard_pca(int_content);

    const char resp[] = "OK";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/*
 * Handle OTA file upload
 */
esp_err_t update_post_handler(httpd_req_t *req)
{
    char buf[1000];
    esp_ota_handle_t ota_handle;
    int remaining = req->content_len;

    const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
    ESP_ERROR_CHECK(esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle));

    while (remaining > 0) {
        int recv_len = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));

        // Timeout Error: Just retry
        if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;

        // Serious Error: Abort OTA
        } else if (recv_len <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Protocol Error");
            return ESP_FAIL;
        }

        // Successful Upload: Flash firmware chunk
        if (esp_ota_write(ota_handle, (const void *)buf, recv_len) != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Flash Error");
            return ESP_FAIL;
        }

        remaining -= recv_len;
    }

    // Validate and switch to new OTA image and reboot
    if (esp_ota_end(ota_handle) != ESP_OK || esp_ota_set_boot_partition(ota_partition) != ESP_OK) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validation / Activation Error");
            return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Firmware update complete, rebooting now!\n");

    vTaskDelay(500 / portTICK_PERIOD_MS);
    esp_restart();

    return ESP_OK;
}

/*
 * HTTP Server
 */

httpd_uri_t spiffs_handler = {
    .uri      = "/*",  // Handle any path
    .method   = HTTP_GET,
    .handler  = spiffs_file_handler,
    .user_ctx = NULL
};

httpd_uri_t wificonfiguressid_post = {
    .uri      = "/configure_wifi_ssid",
    .method   = HTTP_POST,
    .handler  = configure_wifissid_post_handler,
    .user_ctx = NULL
};

httpd_uri_t wificonfigurepasswd_post = {
    .uri      = "/configure_wifi_passwd",
    .method   = HTTP_POST,
    .handler  = configure_wifipasswd_post_handler,
    .user_ctx = NULL
};

httpd_uri_t setdisplay_post = {
    .uri      = "/set_display",
    .method   = HTTP_POST,
    .handler  = setdisplay_post_handler,
    .user_ctx = NULL
};


httpd_uri_t update_post = {
    .uri      = "/update",
    .method   = HTTP_POST,
    .handler  = update_post_handler,
    .user_ctx = NULL
};

httpd_uri_t run_command_post = {
    .uri      = "/run_command",
    .method   = HTTP_POST,
    .handler  = run_command_post_handler,
    .user_ctx = NULL
};

httpd_uri_t restart_get = {
    .uri      = "/restart",
    .method   = HTTP_GET,
    .handler  = restart_get_handler,
    .user_ctx = NULL
};

httpd_uri_t version_get = {
    .uri      = "/version",
    .method   = HTTP_GET,
    .handler  = version_get_handler,
    .user_ctx = NULL
};

static esp_err_t http_server_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 100,
        .format_if_mount_failed = false
    };

    // Mount SPIFFS file system
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE("SPIFFS", "Could not mount SPIFFS filesystem");
        return NULL;
    }
    ESP_LOGI("SPIFFS", "Mounted SPIFFS filesystem");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = 15;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&http_server, &config) == ESP_OK) {
        httpd_register_uri_handler(http_server, &spiffs_handler);
        httpd_register_uri_handler(http_server, &restart_get);
        httpd_register_uri_handler(http_server, &wificonfiguressid_post);
        httpd_register_uri_handler(http_server, &wificonfigurepasswd_post);
        httpd_register_uri_handler(http_server, &run_command_post);
        httpd_register_uri_handler(http_server, &setdisplay_post);
        httpd_register_uri_handler(http_server, &version_get);
    }

    return http_server == NULL ? ESP_FAIL : ESP_OK;
}

static esp_err_t softap_init(char wifi_ssid_ota[10])
{
    esp_err_t res = ESP_OK;

    res |= esp_netif_init();
    res |= esp_event_loop_create_default();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    res |= esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {0};
    wifi_config.ap.channel = 6;
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    wifi_config.ap.max_connection = 3;
    strcpy((char *)wifi_config.ap.ssid, wifi_ssid_ota);
    wifi_config.ap.ssid_len = strlen(wifi_ssid_ota);
    
    res |= esp_wifi_set_mode(WIFI_MODE_AP);
    res |= esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
    res |= esp_wifi_start();

    return res;
}

void flash_leds(void) {
    while (true) {
        setDisplay(node_id, 1);
        ws2811_set_all(20, 0, 0);
        ws2811_set_leds();
        vTaskDelay(250 / portTICK_PERIOD_MS);
        setDisplay(node_id, 0);
        ws2811_set_all(20, 20, 0);
        ws2811_set_leds();
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void start_led_flash_thread(pthread_attr_t attr) {
    pthread_t ledflash_thread;
    int res;
    res = pthread_create(&ledflash_thread, &attr, flash_leds, NULL);
}

void ota(pthread_attr_t attr) {
    if (wifi_connected != true) {
        node_id = random_number(1, 999);
        char wifi_ssid_ota[10];
        sprintf(wifi_ssid_ota, "Node %d", node_id);
        start_led_flash_thread(attr);
        softap_init(wifi_ssid_ota);
        ESP_ERROR_CHECK(http_server_init());
    } else {
        ws2811_set_all(20, 20, 0);
        ws2811_set_leds();
        httpd_register_uri_handler(http_server, &update_post);

        const esp_partition_t *partition = esp_ota_get_running_partition();
        printf("Currently running partition: %s\r\n", partition->label);

        esp_ota_img_states_t ota_state;
        if (esp_ota_get_state_partition(partition, &ota_state) == ESP_OK) {
            if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
                esp_ota_mark_app_valid_cancel_rollback();
            }
        }
    }

    while(1) vTaskDelay(10);
}
