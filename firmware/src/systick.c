/**
 * @file systick.c
 * @brief Base de temps 1 ms via SysTick (AHB 168 MHz, libopencm3).
 */

#include "systick.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

/** @brief Millisecondes écoulées depuis l'init. Volatile : modifié en ISR. */
static volatile uint32_t tick_ms = 0;

/**
 * @brief Configure SysTick à 1 ms (reload = 168 000 − 1, source AHB).
 * @pre   AHB à 168 MHz, interruptions activées.
 */
void systick_init(void)
{
    systick_set_reload(168000 - 1);              /* 168 MHz / 1000 Hz − 1  */
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    systick_interrupt_enable();
}

/** @brief ISR SysTick — incrémente tick_ms toutes les 1 ms. */
void sys_tick_handler(void)
{
    tick_ms++;
}

/**
 * @brief Délai bloquant (busy-wait).
 * @param[in] ms  Durée en millisecondes.
 * @warning Ne pas appeler depuis une ISR.
 */
void delay_ms(uint32_t ms)
{
    uint32_t start = tick_ms;
    while ((tick_ms - start) < ms);
}