#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"

#define TAG "SNIFFER"

// Fonction pour afficher les données des paquets en hexadécimal
static void print_packet_data(const uint8_t *data, int len) {
    printf("Packet data:\n");
    for (int i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) { // Nouvelle ligne toutes les 16 octets
            printf("\n");
        }
    }
    printf("\n");
}

// Handler pour les paquets capturés
static void wifi_sniffer_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    // Afficher la longueur du paquet capturé
    ESP_LOGI(TAG, "Captured packet of length: %d", pkt->rx_ctrl.sig_len);

    // Afficher les données brutes du paquet
    print_packet_data(pkt->payload, pkt->rx_ctrl.sig_len);

    // Exemple de filtrage : afficher uniquement les paquets de type MGMT
    if (type == WIFI_PKT_MGMT) {
        ESP_LOGI(TAG, "Management packet detected.");
    }
}

// Initialisation du mode promiscuous
void sniffer_init() {
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
    ESP_LOGI(TAG, "Sniffer initialized and running.");
}
