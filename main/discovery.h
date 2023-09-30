static char *TAG_DISCOVERY = "DISCOVERY";

void udp_server_thread(void) {
    char rx_buffer[128];
    char addr_str_udp[128];
    int addr_family_udp = AF_INET;
    int ip_protocol_udp = 0;

    struct sockaddr_in dest_addr_udp;
    dest_addr_udp.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr_udp.sin_family = AF_INET;
    dest_addr_udp.sin_port = htons(3344);
    ip_protocol_udp = IPPROTO_IP;

    int listen_sock_udp = socket(addr_family_udp, SOCK_DGRAM, ip_protocol_udp);
    if (listen_sock_udp < 0) {
        ESP_LOGE(TAG_DISCOVERY, "Unable to create socket: errno %d", errno);
        return;
    }

    int err = bind(listen_sock_udp, (struct sockaddr *)&dest_addr_udp, sizeof(dest_addr_udp));
    if (err != 0) {
        ESP_LOGE(TAG_DISCOVERY, "Socket unable to bind: errno %d", errno);
        close(listen_sock_udp);
        return;
    }
    ESP_LOGI(TAG_DISCOVERY, "Discovery thread started");
    while (1) {
        struct sockaddr_in6 source_addr_udp;
        socklen_t socklen = sizeof(source_addr_udp);
        int len = recvfrom(listen_sock_udp, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr_udp, &socklen);
        if (entering_ota == true) {
            break;
        }
        if (rx_connected == true || tx_connected == true) {
            break;
        }
        if (len < 0) {
            ESP_LOGE(TAG_DISCOVERY, "recvfrom failed: errno %d", errno);
            break;
        } else {
            rx_buffer[len] = 0; // Null-terminate
            if (strcmp(rx_buffer, "NODE_DISCOVERY") == 0) {
                ESP_LOGI(TAG_DISCOVERY, "Recieved discovery request, responding..");
                sendto(listen_sock_udp, "NODE_RESPONSE", strlen("NODE_RESPONSE"), 0, (struct sockaddr *)&source_addr_udp, socklen);

            }
        }
    }
    ESP_LOGI(TAG_DISCOVERY, "Discovery thread ending");
    close(listen_sock_udp);
}

void start_discovery_thread(pthread_attr_t attr) {
    pthread_t discoverythread;
    int res;
    res = pthread_create(&discoverythread, &attr, udp_server_thread, NULL);
}
