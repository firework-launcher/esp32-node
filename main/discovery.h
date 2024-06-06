static char *TAG_DISCOVERY = "DISCOVERY";

bool is_ip_discovered(const char *ip_address) {
    for (int i = 0; i < discovered_count; i++) {
        if (strcmp(discovered[i], ip_address) == 0) {
            return true;
        }
    }
    return false;
}

void add_ip_to_discovered(const char *ip_address) {
    if (!is_ip_discovered(ip_address)) {
        if (discovered_count < 128) {
            strncpy(discovered[discovered_count], ip_address, 15);
            discovered[discovered_count][15] = '\0'; // Ensure null termination
            discovered_count++;
        } else {
            ESP_LOGE(TAG_DISCOVERY, "Discovered IP list is full");
        }
    }
}

cJSON* get_discovered(void) {
    cJSON* discovered_json = cJSON_CreateArray();
    for (int i = 0; i < discovered_count; i++) {
        cJSON_AddItemToArray(discovered_json, cJSON_CreateString(discovered[i]));
    }
    return discovered_json;
}

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
        struct sockaddr_in source_addr_udp;
        socklen_t socklen = sizeof(source_addr_udp);
        int len = recvfrom(listen_sock_udp, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr_udp, &socklen);
        if (entering_ota == true) {
            break;
        }
        if (len < 0) {
            ESP_LOGE(TAG_DISCOVERY, "recvfrom failed: errno %d", errno);
            break;
        } else {
            rx_buffer[len] = 0; // Null-terminate
            char ip_address[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(source_addr_udp.sin_addr), ip_address, INET_ADDRSTRLEN);
            if (strcmp(rx_buffer, "NODE_DISCOVERY") == 0) {
                ESP_LOGI(TAG_DISCOVERY, "Received discovery broadcast from %s", ip_address);
                add_ip_to_discovered(ip_address);
            }
        }
    }
    ESP_LOGI(TAG_DISCOVERY, "Discovery thread ending");
    close(listen_sock_udp);
}


void udp_broadcast_thread(void)
{
    struct sockaddr_in broadcast_addr;
    int sock;
    char *message = "NODE_DISCOVERY";

    // Create a UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG_DISCOVERY, "Unable to create socket: errno %d", errno);
        return;
    }

    // Enable broadcast
    int broadcast_perm = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_perm, sizeof(broadcast_perm)) < 0) {
        ESP_LOGE(TAG_DISCOVERY, "Failed to set broadcast option: errno %d", errno);
        close(sock);
        return;
    }

    // Configure the broadcast address
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(3344);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    while (1) {
        if (entering_ota == true) {
            break;
        }
        // Send the broadcast message
        int err = sendto(sock, message, strlen(message), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
        
        if (err < 0) {
            ESP_LOGE(TAG_DISCOVERY, "Error occurred during sending: errno %d", errno);
        } else {
            ESP_LOGI(TAG_DISCOVERY, "Message sent");
        }

        // Wait for the next broadcast interval
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    // Clean up
    close(sock);
}

void start_discovery_thread(pthread_attr_t attr) {
    pthread_t discoverythread;
    int res;
    res = pthread_create(&discoverythread, &attr, udp_server_thread, NULL);
}

void start_discovery_broadcast_thread(pthread_attr_t attr) {
    pthread_t discoverybroadcastthread;
    int res;
    res = pthread_create(&discoverybroadcastthread, &attr, udp_broadcast_thread, NULL);
}
