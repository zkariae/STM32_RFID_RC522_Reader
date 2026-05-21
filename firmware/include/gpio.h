/**
 * @file gpio.h
 * @brief Interface du driver GPIO (libopencm3) — USART2, SPI2 et contrôle RC522.
 */

#ifndef GPIO_H
#define GPIO_H

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

/**
 * @brief Initialise USART2 (PA2/PA3), SPI2 (PB10, PC2/PC3), CS (PB4) et RST (PB5).
 * @pre   Aucun prérequis
 */
void gpio_driver_init(void);

#endif /* GPIO_H */
