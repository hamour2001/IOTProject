#ifndef DNS_SPOOF_H
#define DNS_SPOOF_H

#include <stdint.h>

// Configuration
#define DNS_PORT 53
#define SPOOFED_IP "192.168.4.1"
#define TARGET_DOMAIN "infrastructure.local"

// Prototypes
void dns_server_task(void *pvParameters);

#endif // DNS_SPOOF_H
