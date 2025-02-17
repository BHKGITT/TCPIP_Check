#include <string.h>
#include "lwip/tcp.h"
#include "lwip/pbuf.h"

#define CMD_WRITE 0x01
#define CMD_READ  0x02
#define ACK_BYTE  0x06

// Buffer to store data written from the backend.
static uint8_t storedData[256];
static uint8_t storedDataLength = 0;

err_t recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL) {
        // Connection closed
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Assume the complete packet is contained in one pbuf.
    uint8_t *rx_data = (uint8_t*)p->payload;
    uint16_t rx_len = p->len;

    if (rx_len >= 1) {
        uint8_t command = rx_data[0];
        if (command == CMD_WRITE) {
            if (rx_len >= 2) {
                uint8_t len = rx_data[1];
                if (rx_len >= (uint16_t)(2 + len)) {
                    // Store received data.
                    storedDataLength = len;
                    memcpy(storedData, &rx_data[2], len);
                    // Send acknowledgment (ACK)
                    tcp_write(tpcb, &ACK_BYTE, 1, TCP_WRITE_FLAG_COPY);
                }
            }
        }
        else if (command == CMD_READ) {
            // Prepare response packet: first byte is the length, then data.
            uint8_t response[257];
            response[0] = storedDataLength;
            memcpy(&response[1], storedData, storedDataLength);
            tcp_write(tpcb, response, storedDataLength + 1, TCP_WRITE_FLAG_COPY);
        }
    }

    // Free the received pbuf.
    pbuf_free(p);
    return ERR_OK;
}
