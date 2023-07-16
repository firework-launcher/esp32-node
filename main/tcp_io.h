#include <string.h>
#include <sys/param.h>
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "pthread.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "fireworks.h"

static const char *TAG_TCP_TX = "TCP TX";
static const char *TAG_TCP_RX = "TCP RX";
char msg_buffer_input[128];
bool tx_connected = false;
bool rx_connected = false;

static void tcp_inserver_thread(void)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    

    while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(4444);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0) {
            ESP_LOGE(TAG_TCP_TX, "Unable to create socket: errno %d", errno);
            break;
        }

        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG_TCP_TX, "Socket unable to bind: errno %d", errno);
            break;
        }

        err = listen(listen_sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG_TCP_TX, "Error occurred during listen: errno %d", errno);
            break;
        }

        struct sockaddr_storage source_addr;
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG_TCP_TX, "Unable to accept connection: errno %d", errno);
            break;
        }
        tx_connected = true;
        while (1) {
            if (strcmp(msg_buffer_input, "") != 0) {
                send(sock, msg_buffer_input, strlen(msg_buffer_input), 0);
                strcpy(msg_buffer_input, "");
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG_TCP_TX, "Connection to controller lost.");
            shutdown(sock, 0);
            close(sock);
            tx_connected = false;
        }
    }
}

void process_rx_data(char rx_data[128]) {
    char recv_data[128];
    memcpy(recv_data, rx_data, strlen(rx_data)+1);
    char *data = strtok(recv_data, "\n");
    char *cmd;
    char *cmd_cpy;
    char full_msg[128] = "";
    while (data != NULL) {
        free(cmd);
        char cmd[128] = strtok(data, "\x80");
        while (cmd != NULL)  {
            printf(cmd);
            if (strcmp(cmd, "") != 0) {
                int code = cmd[0];
                char full_cmd[128];
                char values[128];
                memcpy(values, cmd, strlen(cmd)+1);
                memmove(values, values+1, strlen(values));
                char *value = strtok(values, "\x7f");
                char values_formatted[128] = "[";
                int x = 0;
                while (value != NULL) {
                    if (x != 0) {
                        strcat(values_formatted, ", ");
                    }
                    if (code == 130) {
                        launch_firework(atoi(value));
                    }
                    strcat(values_formatted, value);
                    value = strtok(NULL, "\x7f");
                    x += 1;
                }
                strcat(values_formatted, "]");
                char *instruction_name;
                switch (code) {
                    case 130:
                        instruction_name = "Trigger Firework";
                        break;
                    case 132:
                        instruction_name = "Start Pattern Data";
                        break;
                    case 133:
                        instruction_name = "End Pattern Data";
                        break;
                    case 134:
                        instruction_name = "Run Pattern";
                        break;
                    default:
                        instruction_name = "Unrecognized Command";
                        break;
                }
                ESP_LOGI(TAG_TCP_RX, "Recived, %s, Values: %s", instruction_name, values_formatted);
            }
            cmd = strtok(NULL, "\x80");
        }
        data = strtok(NULL, "\n");
    }
}

static void tcp_outserver_thread(void)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    char recv_buffer[128];
    int len;

    while (1) {
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(3333);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0) {
            ESP_LOGE(TAG_TCP_RX, "Unable to create socket: errno %d", errno);
            break;
        }

        int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            if (err != 112) {
                ESP_LOGE(TAG_TCP_RX, "Socket unable to bind: errno %d", errno);
                break;
            }
        }

        err = listen(listen_sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG_TCP_RX, "Error occurred during listen: errno %d", errno);
            break;
        }

        struct sockaddr_storage source_addr;
        uint addr_len = sizeof(source_addr);
        while (1) {
            int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if (sock < 0) {
                ESP_LOGE(TAG_TCP_RX, "Unable to accept connection: errno %d", errno);
                break;
            }
            rx_connected = true;
            while (1) {
                len = recv(sock, &recv_buffer, sizeof(recv_buffer) - 1, 0);
                recv_buffer[len] = 0;
                if (len != 0) {
                    if (len < 0) {
                        ESP_LOGE(TAG_TCP_RX, "Failed to recieve, closing connection.");
                        ESP_LOGE(TAG_TCP_RX, "errno %d", len);
                        shutdown(sock, 0);
                        close(sock);
                        ESP_LOGE(TAG_TCP_RX, "Connection to controller lost.");
                        rx_connected = false;
                        break;
                    }
                    ESP_LOGI(TAG_TCP_RX, "len var: %d", len);
                    ESP_LOGI(TAG_TCP_RX, "New message from TCP RX: %s", recv_buffer);
                    process_rx_data(recv_buffer);
                }
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
    }
}

void start_tcptx_thread(void) {
    pthread_t tcpthread;
    int res;

    res = pthread_create(&tcpthread, NULL, tcp_inserver_thread, NULL);
    ESP_LOGI(TAG_TCP_TX, "Started TCP TX Thread");
}

void start_tcprx_thread(void) {
    pthread_t tcpthread;
    int res;

    res = pthread_create(&tcpthread, NULL, tcp_outserver_thread, NULL);
    ESP_LOGI(TAG_TCP_RX, "Started TCP RX Thread");
}
