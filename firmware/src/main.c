#include "gpio.h"
#include "uart.h"
#include "systick.h"
#include "log.h"

int main(void)
{
    systick_init();
    gpio_driver_init();
    uart_init();

    LOG_INFO("=== STM32F407 RFID RC522 ===");
    LOG_INFO("System OK");

    while (1) {
        /* TODO: RFID RC522 driver */
    }

    return 0;
}