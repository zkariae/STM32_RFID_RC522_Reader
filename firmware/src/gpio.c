/**
 * @file gpio.c
 * @brief Driver GPIO — USART2 (PA2/PA3) et SPI2 (PB13-15) via libopencm3.
 */

#include "gpio.h"

/**
 * @brief Initialise les broches GPIO pour USART2 et SPI2.
 * @pre   À appeler avant uart_init() et spi_driver_init().
 */
void gpio_driver_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

    /* PA2 (TX), PA3 (RX) — AF7 (USART2) */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

    /* PB4 (NSS) — sortie push-pull, idle HIGH (RC522 désélectionné) */
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO4);
    gpio_set(GPIOB, GPIO4);

    /* PB13 (SCK), PB14 (MISO), PB15 (MOSI) — AF5 (SPI2) */
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO13 | GPIO14 | GPIO15);
    gpio_set_af(GPIOB, GPIO_AF5, GPIO13 | GPIO14 | GPIO15);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO13 | GPIO15);
}