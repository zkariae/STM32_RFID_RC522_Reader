/**
 * @file gpio.c
 * @brief Driver GPIO — USART2 (PA2/PA3) et SPI1 (PA4–PA7) via libopencm3.
 */

#include "gpio.h"

/**
 * @brief Initialise les broches GPIOA pour USART2 et SPI1.
 * @pre   À appeler avant uart_init() et spi_driver_init().
 */
void gpio_driver_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);

    /* PA2 (TX), PA3 (RX) — AF7 (USART2) */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

    /* PA4 (NSS) — sortie push-pull, idle HIGH (RC522 désélectionné) */
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO4);
    gpio_set(GPIOA, GPIO4);

    /* PA5 (SCK), PA6 (MISO), PA7 (MOSI) — AF5 (SPI1) */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO5 | GPIO6 | GPIO7);
    gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO5 | GPIO7); /* SCK + MOSI uniquement */
}