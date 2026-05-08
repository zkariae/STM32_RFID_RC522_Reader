#include "gpio.h"
#include "uart.h"
#include "systick.h"
#include "log.h"
#include "spi.h"
#include "rfid_rc522.h"

int main(void)
{
    systick_init();
    gpio_driver_init();
    uart_init();
    spi_driver_init();

    uart_send_string("=== Test Lecture Multiple ===\r\n");

    /* Reset hardware RC522 via RST pin */
    RC522_RST_HIGH();   /* RST = 1 */
    RC522_CS_HIGH();    /* CS = 1 */
    for (volatile int i = 0; i < 200000; i++) { }
    
    RC522_RST_LOW();    /* RST = 0 -> reset */
    for (volatile int i = 0; i < 200000; i++) { }
    
    RC522_RST_HIGH();   /* RST = 1 -> fin reset */
    for (volatile int i = 0; i < 2000000; i++) { }  /* Attendre RC522 pret */

    /* Lecture version 10 fois - avec plus de delai */
    uart_send_string("Lecture version 10 fois:\r\n");
    for (int i = 0; i < 10; i++) {
        RC522_CS_LOW();
        for (volatile int j = 0; j < 100000; j++) { }  /* Delai plus long */
        rc522_spi_write(0x00);
        for (volatile int j = 0; j < 20000; j++) { }
        rc522_spi_write(0x80 | 0x37);
        for (volatile int j = 0; j < 20000; j++) { }
        uint8_t v = rc522_spi_read();
        for (volatile int j = 0; j < 20000; j++) { }
        RC522_CS_HIGH();
        for (volatile int j = 0; j < 100000; j++) { }

        uart_send_string("Essai ");
        uart_send_int(i + 1);
        uart_send_string(": ");
        uart_send_hex(v);
        uart_send_string("\r\n");
    }

    /* Lecture Status2 (0x08) 5 fois - avec plus de delai */
    uart_send_string("\r\nLecture Status2 (0x08) 5 fois:\r\n");
    for (int i = 0; i < 5; i++) {
        RC522_CS_LOW();
        for (volatile int j = 0; j < 100000; j++) { }
        rc522_spi_write(0x00);
        for (volatile int j = 0; j < 20000; j++) { }
        rc522_spi_write(0x80 | 0x08);
        for (volatile int j = 0; j < 20000; j++) { }
        uint8_t v = rc522_spi_read();
        for (volatile int j = 0; j < 20000; j++) { }
        RC522_CS_HIGH();
        for (volatile int j = 0; j < 100000; j++) { }
        
        uart_send_string("Essai ");
        uart_send_int(i + 1);
        uart_send_string(": ");
        uart_send_hex(v);
        uart_send_string("\r\n");
    }

    /* Test avec driver RFID existant */
    uart_send_string("\r\nTest detection avec driver:\r\n");

    /* Reset hardware */
    RC522_RST_LOW();
    for (volatile int i = 0; i < 100000; i++) { }
    RC522_RST_HIGH();
    for (volatile int i = 0; i < 500000; i++) { }

    /* Init RC522 via driver */
    RC522_Status status = rfid_rc522_init();
    
    uart_send_string("Init status: ");
    uart_send_hex(status);
    uart_send_string("\r\n");
    
    if (status == RC522_STATUS_OK) {
        uart_send_string("RC522 init OK\r\n");
        
        /* Request card - plusieurs tentatives */
        for (int i = 0; i < 3; i++) {
            uart_send_string("Essai ");
            uart_send_int(i + 1);
            uart_send_string("\r\n");
            
            uint8_t atqa[2];
            status = rfid_rc522_request(atqa);
            
            uart_send_string("Request status: ");
            uart_send_hex(status);
            uart_send_string("\r\n");
            
            if (status == RC522_STATUS_OK) {
                uart_send_string("Carte detectee!\r\n");
                uart_send_string("ATQA: ");
                uart_send_hex(atqa[0]);
                uart_send_string(" ");
                uart_send_hex(atqa[1]);
                uart_send_string("\r\n");
                break;
            } else {
                uart_send_string("Pas de carte\r\n");
            }
            
            for (volatile int j = 0; j < 500000; j++) { }
        }
    } else {
        uart_send_string("RC522 init ERREUR\r\n");
    }

    uart_send_string("\r\nFini\r\n");

    while (1) { }

    return 0;
}