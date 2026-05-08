#include "gpio.h"
#include "uart.h"
#include "systick.h"
#include "log.h"
#include "rfid_rc522.h"

/* Cle par defaut pour MIFARE Classic (FFFFFFFFFFFF) */
static const RC522_Key default_key = {
    .key = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
};

int main(void)
{
    RC522_Status status;

    systick_init();
    gpio_driver_init();
    uart_init();
    spi_driver_init();

    uart_send_string("=== STM32F407 RFID RC522 ===\r\n");

    /* Initialise le RC522 */
    status = rfid_rc522_init();
    if (status != RC522_STATUS_OK) {
        uart_send_string("Echec init RC522\r\n");
        while (1) { }
    }

    uart_send_string("RC522 pret!\r\n");
    uart_send_string("Approchez une carte...\r\n");

    while (1) {
        /* Detection de carte */
        uint8_t atqa[2];
        status = rfid_rc522_request(atqa);

        if (status == RC522_STATUS_OK) {
            uart_send_string("Carte detectee\r\n");

            /* Anti-collision */
            RC522_UID uid;
            status = rfid_rc522_anticoll(&uid);

            if (status == RC522_STATUS_OK) {
                uart_send_string("UID: ");
                for (int i = 0; i < uid.size; i++) {
                    uint8_t val = uid.uid[i];
                    uart_send_string("0x");
                    uart_send_int(val);
                    uart_send_string(" ");
                }
                uart_send_string("\r\n");

                /* Selection */
                status = rfid_rc522_select(&uid);
                if (status == RC522_STATUS_OK) {
                    uart_send_string("SAK: ");
                    uart_send_int(uid.sak);
                    uart_send_string("\r\n");

                    /* Verifie Crypto1 status */
                    uint8_t crypto = rfid_rc522_get_crypto_status();
                    uart_send_string("Crypto: ");
                    uart_send_int(crypto);
                    uart_send_string("\r\n");

                    /* Authentification et lecture du bloc 0 */
                    status = rfid_rc522_auth(0, PICC_CMD_MIFARE_AUTH_KEY_A, &default_key, &uid);
                    if (status == RC522_STATUS_OK) {
                        uart_send_string("Auth OK\r\n");

                        uint8_t block_data[16];
                        status = rfid_rc522_read_block(0, block_data);
                        if (status == RC522_STATUS_OK) {
                            uart_send_string("Bloc 0 lu:\r\n");
                            for (int i = 0; i < 16; i++) {
                                uart_send_int(block_data[i]);
                                uart_send_string(" ");
                                if (i == 7) uart_send_string("\r\n");
                            }
                            uart_send_string("\r\n");
                        } else {
                            uart_send_string("Erreur lecture\r\n");
                        }
                    } else {
                        uart_send_string("Erreur auth\r\n");
                    }
                } else {
                    uart_send_string("Erreur select\r\n");
                }
            }

            rfid_rc522_halt();
        }

        /* Delai */
        for (volatile int i = 0; i < 500000; i++) { }
    }

    return 0;
}