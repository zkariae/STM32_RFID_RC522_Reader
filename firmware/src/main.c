/**
 * main.c
 * Test UART — envoi d'une trame en boucle
 */

#include "gpio.h"
#include "uart.h"
#include "systick.h"

int main(void)
{
    gpio_driver_init();
    systick_init();
    uart_init();

    uart_send_string("=== STM32F407 RFID RC522 ===\r\n");
    uart_send_string("[INIT] System OK\r\n");

    while (1) {
        uart_send_string("[UART] Hello from STM32F407!\r\n");
        delay_ms(1000);
    }

    return 0;
}