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

    /* Test detection carte - avec verification erreur */
    uart_send_string("\r\nTest detection carte:\r\n");

    /* Clear IRQ et FIFO */
    RC522_CS_LOW();
    for (volatile int j = 0; j < 100000; j++) { }
    rc522_spi_write(0x00);
    for (volatile int j = 0; j < 20000; j++) { }
    rc522_spi_write(0x04);  /* CommIrq reg */
    for (volatile int j = 0; j < 20000; j++) { }
    rc522_spi_write(0x7F);  /* Clear all IRQ */
    for (volatile int j = 0; j < 30000; j++) { }
    RC522_CS_HIGH();
    for (volatile int j = 0; j < 100000; j++) { }

    /* Config RxMode - 106kbps */
    RC522_CS_LOW();
    for (volatile int j = 0; j < 100000; j++) { }
    rc522_spi_write(0x00);
    for (volatile int j = 0; j < 20000; j++) { }
    rc522_spi_write(0x13);  /* RxMode reg */
    for (volatile int j = 0; j < 20000; j++) { }
    rc522_spi_write(0x00);  /* No CRC */
    for (volatile int j = 0; j < 30000; j++) { }
    RC522_CS_HIGH();
    for (volatile int j = 0; j < 100000; j++) { }

    /* Send REQA */
    RC522_CS_LOW();
    for (volatile int j = 0; j < 100000; j++) { }
    rc522_spi_write(0x00);
    for (volatile int j = 0; j < 20000; j++) { }
    rc522_spi_write(0x09);  /* FIFO */
    for (volatile int j = 0; j < 20000; j++) { }
    rc522_spi_write(0x26);  /* REQA */
    for (volatile int j = 0; j < 30000; j++) { }
    rc522_spi_write(0x01);  /* Command */
    for (volatile int j = 0; j < 20000; j++) { }
    rc522_spi_write(0x0C);  /* Transceive */
    for (volatile int j = 0; j < 100000; j++) { }
    
    /* Wait */
    for (volatile int j = 0; j < 200000; j++) { }
    
    /* Check IRQ */
    rc522_spi_write(0x80 | 0x04);  /* CommIrq */
    for (volatile int j = 0; j < 20000; j++) { }
    uint8_t irq = rc522_spi_read();
    for (volatile int j = 0; j < 20000; j++) { }
    
    /* Check Error reg */
    rc522_spi_write(0x80 | 0x06);  /* Error reg */
    for (volatile int j = 0; j < 20000; j++) { }
    uint8_t err = rc522_spi_read();
    for (volatile int j = 0; j < 20000; j++) { }
    RC522_CS_HIGH();

    uart_send_string("CommIrq: ");
    uart_send_hex(irq);
    uart_send_string("\r\n");
    uart_send_string("Error: ");
    uart_send_hex(err);
    uart_send_string("\r\n");

    if (irq & 0x30) {
        uart_send_string("Carte detectee!\r\n");
    } else {
        uart_send_string("Pas de carte\r\n");
    }

    uart_send_string("\r\nFini\r\n");

    while (1) { }

    return 0;
}