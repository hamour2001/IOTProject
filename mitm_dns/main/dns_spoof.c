#include "dns_spoof.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/ip_addr.h"

static const char *TAG = "DNS_SPOOF";

typedef struct __attribute__((packed)) {
    uint16_t id;     // Identifier
    uint16_t flags;  // DNS Flags
    uint16_t qdcount; // Question Count
    uint16_t ancount; // Answer Count
    uint16_t nscount; // Authority Count
    uint16_t arcount; // Additional Count
} dns_header_t;

void dns_server_task(void *pvParameters) {
    struct sockaddr_in server_addr, client_addr;
    char buffer[512];
    int sock, len;
    socklen_t addr_len = sizeof(client_addr);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket creation failed.");
        vTaskDelete(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Socket binding failed.");
        close(sock);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "DNS spoofing server running on port %d", DNS_PORT);

    while (1) {
        len = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (len < 0) {
            ESP_LOGE(TAG, "Error receiving data.");
            continue;
        }

        dns_header_t *header = (dns_header_t *)buffer;
        header->flags = htons(0x8180); // Response flag with no error
        header->ancount = htons(1);   // One answer

        uint8_t *query = (uint8_t *)(buffer + sizeof(dns_header_t));
        int query_len = len - sizeof(dns_header_t);

        uint8_t *response = query + query_len;
        memcpy(response, query, query_len); // Copy the query to the response
        response += query_len;

        *response++ = 0xC0; // Pointer to the query
        *response++ = 0x0C;
        *(uint16_t *)response = htons(0x0001); // Type A
        response += 2;
        *(uint16_t *)response = htons(0x0001); // Class IN
        response += 2;
        *(uint32_t *)response = htonl(60);    // TTL 60 seconds
        response += 4;
        *(uint16_t *)response = htons(4);     // Data length
        response += 2;

        // Spoofed IP address
        sscanf(SPOOFED_IP, "%hhu.%hhu.%hhu.%hhu", response, response + 1, response + 2, response + 3);

        sendto(sock, buffer, (response - (uint8_t *)buffer) + 4, 0, (struct sockaddr *)&client_addr, addr_len);
        ESP_LOGI(TAG, "DNS query handled, spoofed IP sent: %s", SPOOFED_IP);
    }

    close(sock);
    vTaskDelete(NULL);
}
