#include "gpio.h"
#include "uart.h"
#include "systick.h"
#include "log.h"
#include "spi.h"

int main(void)
{
    systick_init();
    gpio_driver_init();
    uart_init();
    spi_driver_init();

    uart_send_string("=== Test Lecture Multiple ===\r\n");

    /* Reset RC522 - delai plus long pour initialization */
    RC522_CS_HIGH();
    for (volatile int i = 0; i < 100000; i++) { }
    RC522_CS_LOW();
    for (volatile int i = 0; i < 10000; i++) { }
    RC522_CS_HIGH();
    for (volatile int i = 0; i < 500000; i++) { }  /* Delai plus long pour RC522 pret */

    /* Lecture version 5 fois pour verifier la stabilite */
    uart_send_string("Lecture version 5 fois:\r\n");
    for (int i = 0; i < 5; i++) {
        RC522_CS_LOW();
        for (volatile int j = 0; j < 5000; j++) { }
        spi_transfer(0x80 | 0x37);
        for (volatile int j = 0; j < 1000; j++) { }
        uint8_t v = spi_transfer(0x00);
        for (volatile int j = 0; j < 5000; j++) { }
        RC522_CS_HIGH();
        for (volatile int j = 0; j < 5000; j++) { }  /* Delai entre transfers */

        uart_send_string("Essai ");
        uart_send_int(i + 1);
        uart_send_string(": 0x");
        uart_send_int(v);
        uart_send_string("\r\n");

        for (volatile int j = 0; j < 100000; j++) { }
    }

    /* Lecture autres registres pour comparaison */
    uart_send_string("\r\nLecture autres registres:\r\n");

    /* FIFO Level (0x0A) */
    RC522_CS_LOW();
    for (volatile int j = 0; j < 5000; j++) { }
    spi_transfer(0x80 | 0x0A);
    for (volatile int j = 0; j < 1000; j++) { }
    uint8_t fifo = spi_transfer(0x00);
    for (volatile int j = 0; j < 5000; j++) { }
    RC522_CS_HIGH();
    for (volatile int j = 0; j < 20000; j++) { }
    uart_send_string("FIFO Level (0x0A): 0x");
    uart_send_int(fifo);
    uart_send_string("\r\n");

    /* Status 2 (0x08) */
    RC522_CS_LOW();
    for (volatile int j = 0; j < 5000; j++) { }
    spi_transfer(0x80 | 0x08);
    for (volatile int j = 0; j < 1000; j++) { }
    uint8_t status2 = spi_transfer(0x00);
    for (volatile int j = 0; j < 5000; j++) { }
    RC522_CS_HIGH();
    for (volatile int j = 0; j < 20000; j++) { }
    uart_send_string("Status 2 (0x08): 0x");
    uart_send_int(status2);
    uart_send_string("\r\n");

    /* Command (0x01) - should be 0x00 after reset */
    RC522_CS_LOW();
    for (volatile int j = 0; j < 5000; j++) { }
    spi_transfer(0x80 | 0x01);
    for (volatile int j = 0; j < 1000; j++) { }
    uint8_t cmd = spi_transfer(0x00);
    for (volatile int j = 0; j < 5000; j++) { }
    RC522_CS_HIGH();
    for (volatile int j = 0; j < 20000; j++) { }
    uart_send_string("Command (0x01): 0x");
    uart_send_int(cmd);
    uart_send_string("\r\n");

    /* Test ecriture/lecture sur registre Command (0x01) - writable */
    uart_send_string("\r\nTest write/read Command (0x01):\r\n");

    /* Ecrire 0x05 dans Command (bit 2 = idle) */
    RC522_CS_LOW();
    for (volatile int j = 0; j < 5000; j++) { }
    spi_transfer(0x01);  /* Write to 0x01 */
    for (volatile int j = 0; j < 1000; j++) { }
    spi_transfer(0x05);
    for (volatile int j = 0; j < 5000; j++) { }
    RC522_CS_HIGH();
    for (volatile int j = 0; j < 20000; j++) { }

    /* Lire Command */
    RC522_CS_LOW();
    for (volatile int j = 0; j < 5000; j++) { }
    spi_transfer(0x80 | 0x01);
    for (volatile int j = 0; j < 1000; j++) { }
    uint8_t val = spi_transfer(0x00);
    for (volatile int j = 0; j < 5000; j++) { }
    RC522_CS_HIGH();

    uart_send_string("Write 0xAA, Read: 0x");
    uart_send_int(val);
    uart_send_string("\r\n");

    if (val == 0xAA) {
        uart_send_string("SPI OK!\r\n");
    } else {
        uart_send_string("SPI Erreur!\r\n");
    }

    uart_send_string("\r\nFini\r\n");

    while (1) { }

    return 0;
}