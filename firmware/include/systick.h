/**
 * @file systick.h
 * @brief Interface du driver SysTick — base de temps 1 ms (libopencm3).
 */

#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

/** @brief Configure SysTick à 1 ms. @pre AHB à 168 MHz, IRQ activées. */
void systick_init(void);

/** @brief Délai bloquant. @param[in] ms Durée en millisecondes. */
void delay_ms(uint32_t ms);

#endif /* SYSTICK_H */