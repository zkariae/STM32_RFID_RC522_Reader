/**
 * @file gpio.c
 * @brief Driver GPIO — USART2 (PA2/PA3), SPI2 (PB10, PC2/PC3), CS (PB4), et RST (PB5).
 */

#include "gpio.h"

/**
 * @brief Initialise les broches GPIO pour USART2, SPI2, RC522 CS et RST.
 */
void gpio_driver_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);

    /* PA2 (TX), PA3 (RX) — AF7 (USART2) */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

    /* PB4 (CS/NSS), PB5 (RST) — sorties push-pull */
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4 | GPIO5);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO4 | GPIO5);
    gpio_set(GPIOB, GPIO4 | GPIO5);

    /* PB10 (SCK) — AF5 (SPI2) */
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);
    gpio_set_af(GPIOB, GPIO_AF5, GPIO10);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO10);

    /* PC2 (MISO), PC3 (MOSI) — AF5 (SPI2) */
    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO2 | GPIO3);
    gpio_set_af(GPIOC, GPIO_AF5, GPIO2 | GPIO3);
    gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_100MHZ, GPIO2 | GPIO3);
}
