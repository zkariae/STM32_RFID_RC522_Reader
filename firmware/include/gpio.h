/**
 * @file gpio.h
 * @brief Interface du driver GPIO (libopencm3) — USART2 et SPI1 sur GPIOA.
 */

#ifndef GPIO_H
#define GPIO_H

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

/**
 * @brief Initialise les broches GPIOA pour USART2 (PA2/PA3) et SPI1 (PA4–PA7).
 * @pre   Aucun prérequis
 */
void gpio_driver_init(void);

#endif /* GPIO_H */