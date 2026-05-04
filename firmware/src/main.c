#include "gpio.h"
#include "uart.h"
#include "systick.h"
#include "log.h"

typedef enum {
    STATE_DEBUG = 0,
    STATE_INFO,
    STATE_WARN,
    STATE_ERROR
} LogState;

int main(void)
{
    systick_init();
    gpio_driver_init();
    uart_init();

    uart_send_string("=== STM32F407 RFID RC522 ===\r\n");
    uart_send_string("[INIT] System OK\r\n");

    LogState state = STATE_DEBUG;
    uint32_t counter = 0;

    while (1) {
        switch (state) {
            case STATE_DEBUG:
                LOG_DEBUG("SysTick running");
                LOG_DEBUG_INT("tick = ", systick_get_tick());
                state = STATE_INFO;
                break;

            case STATE_INFO:
                LOG_INFO("System nominal");
                LOG_DEBUG_INT("counter = ", counter);
                state = STATE_WARN;
                break;

            case STATE_WARN:
                LOG_WARN("Voltage threshold approaching");
                state = STATE_ERROR;
                break;

            case STATE_ERROR:
                LOG_ERROR("Simulated error triggered");
                counter++;
                state = STATE_DEBUG;
                break;
        }

        delay_ms(1000);
    }

    return 0;
}