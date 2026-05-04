#include "systick.h"

static volatile uint32_t tick_counter = 0;

void systick_init(void)
{
    /* Désactiver COMPLÈTEMENT le FPU context saving
       FPCCR = 0 : désactive LSPEN et ASPEN */
    (*(volatile uint32_t *)0xE000EF34UL) = 0x00000000UL;

    /* Activer FPU CP10/CP11 */
    (*(volatile uint32_t *)0xE000ED88UL) |= (0xFUL << 20);

    /* Configurer SysTick pour 1ms à 16MHz */
    (*(volatile uint32_t *)0xE000E010UL) = 0;     /* CTRL : disable */
    (*(volatile uint32_t *)0xE000E014UL) = 15999; /* LOAD : 16MHz/1000 - 1 */
    (*(volatile uint32_t *)0xE000E018UL) = 0;     /* VAL  : reset */
    (*(volatile uint32_t *)0xE000E010UL) =
        0x00000007; /* CTRL : enable + irq + AHB */
}

void SysTick_Handler(void)
{
    tick_counter++;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = tick_counter;
    while ((tick_counter - start) < ms)
        ;
}

uint32_t systick_get_tick(void)
{
    return tick_counter;
}