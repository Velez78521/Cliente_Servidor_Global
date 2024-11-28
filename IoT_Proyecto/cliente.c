#include <stdio.h>
#include <string.h>
#include "net/af.h"
#include "dht.h"
#include "xtimer.h"
#include "shell.h"
#include "msg.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/ipv6.h"
#include "net/protnum.h"
#include "net/ipv6/addr.h"
#include "net/sock/udp.h"


#define MAIN_QUEUE_SIZE (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
//fe80::eb8:15ff:fef2:ecc1 atupaña
#define DHTPIN 0
#define DHTTYPE DHT11

char buf[128];
dht_t dht;
int16_t temperature, humidity;

void configure_global_ipv6(void) {
    // Interfaz de red
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);

    if (netif == NULL) {
        puts("No network interface found");
        return;
    }

    // Dirección IPv6 global a asignar
    const char *ipv6_addr_str="2001:db8::eb8:15ff:fef2:ecc1";
     //"2001:db8::feb4:67ff:fef1:3c1d";
    ipv6_addr_t ipv6_addr;
    if (ipv6_addr_from_str(&ipv6_addr, ipv6_addr_str) == NULL) {
        puts("Error: unable to parse IPv6 address");
        return;
    }

    if (gnrc_netif_ipv6_addr_add(netif, &ipv6_addr, 64, GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID) < 0) {
        puts("Error: unable to add IPv6 address");
        return;
    }


}

void start_client(void) {

    dht_params_t params = {
        .pin = DHTPIN,
        .type = DHTTYPE
    };

    if (dht_init(&dht, &params) != DHT_OK) {
        printf("Falla al iniciar DHT sensor\n");
        return;
    }

    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    sock_udp_t sock;

    local.port = 54321; // Puerto del cliente, puede ser cualquier número

    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("Error creating UDP sock");
        return;
    }

    while (1) {
        if (dht_read(&dht, &temperature, &humidity) != DHT_OK) {
            printf(" ");
            continue;
        }
        float temperature_C = temperature / 10.0f;
        float humidity_float = humidity / 10.0f;
        sprintf(buf, "T: %.2f°C, H: %.2f%%", temperature_C, humidity_float);

        // Enviar datos al servidor UDP
        sock_udp_ep_t remote = { .family = AF_INET6 };
        remote.port = 6969; // Puerto del servidor
        // Dirección IPv6 del servidor
        ipv6_addr_from_str((ipv6_addr_t *)&remote.addr.ipv6, "2001:db8:cb::1");
//fe80::fee8:c0ff:fe74:a3d1
        if (sock_udp_send(&sock, buf, strlen(buf), &remote) < 0) {
            puts("Fallo el envio de datos");
            sock_udp_close(&sock);
            return;
        }
	printf("Enviando datos...");
        xtimer_sleep(10);
    }
}

int main(void) {
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    configure_global_ipv6();
    start_client();
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* Esto nunca debería alcanzarse */
    return 0;
}
